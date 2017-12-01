#include "androidPlatform.h"

#include "data/properties.h"
#include "data/propertyItem.h"
#include "log.h"
#include "map.h"
#include "util/url.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
#include <android/log.h>
#include <android/asset_manager_jni.h>
#include <cstdarg>
#include <dlfcn.h> // dlopen, dlsym
#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <codecvt>
#include <locale>

#include "sqlite3ndk.h"


PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT = 0;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT = 0;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT = 0;

namespace Tangram {

/* Followed the following document for JavaVM tips when used with native threads
 * http://android.wooyd.org/JNIExample/#NWD1sCYeT-I
 * http://developer.android.com/training/articles/perf-jni.html and
 * http://www.ibm.com/developerworks/library/j-jni/
 * http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/invocation.html
 */

static JavaVM* jvm = nullptr;
// JNI Env bound on androids render thread (our native main thread)
static jmethodID requestRenderMethodID = 0;
static jmethodID setRenderModeMethodID = 0;
static jmethodID startUrlRequestMID = 0;
static jmethodID cancelUrlRequestMID = 0;
static jmethodID getFontFilePath = 0;
static jmethodID getFontFallbackFilePath = 0;
static jmethodID onFeaturePickMID = 0;
static jmethodID onLabelPickMID = 0;
static jmethodID onMarkerPickMID = 0;
static jmethodID labelPickResultInitMID = 0;
static jmethodID markerPickResultInitMID = 0;
static jmethodID sceneReadyCallbackMID = 0;
static jmethodID sceneErrorInitMID = 0;

static jclass labelPickResultClass = nullptr;
static jclass sceneErrorClass = nullptr;
static jclass markerPickResultClass = nullptr;

static jclass hashmapClass = nullptr;
static jmethodID hashmapInitMID = 0;
static jmethodID hashmapPutMID = 0;

static jmethodID markerByIDMID = 0;

static bool glExtensionsLoaded = false;

void AndroidPlatform::bindJniEnvToThread(JNIEnv* jniEnv) {
    jniEnv->GetJavaVM(&jvm);
}

void AndroidPlatform::setupJniEnv(JNIEnv* jniEnv) {
    bindJniEnvToThread(jniEnv);

    jclass tangramClass = jniEnv->FindClass("com/mapzen/tangram/MapController");
    startUrlRequestMID = jniEnv->GetMethodID(tangramClass, "startUrlRequest", "(Ljava/lang/String;J)V");
    cancelUrlRequestMID = jniEnv->GetMethodID(tangramClass, "cancelUrlRequest", "(J)V");
    getFontFilePath = jniEnv->GetMethodID(tangramClass, "getFontFilePath", "(Ljava/lang/String;)Ljava/lang/String;");
    getFontFallbackFilePath = jniEnv->GetMethodID(tangramClass, "getFontFallbackFilePath", "(II)Ljava/lang/String;");
    requestRenderMethodID = jniEnv->GetMethodID(tangramClass, "requestRender", "()V");
    setRenderModeMethodID = jniEnv->GetMethodID(tangramClass, "setRenderMode", "(I)V");
    sceneReadyCallbackMID = jniEnv->GetMethodID(tangramClass, "sceneReadyCallback", "(ILcom/mapzen/tangram/SceneError;)V");

    jclass featurePickListenerClass = jniEnv->FindClass("com/mapzen/tangram/MapController$FeaturePickListener");
    onFeaturePickMID = jniEnv->GetMethodID(featurePickListenerClass, "onFeaturePick", "(Ljava/util/Map;FF)V");
    jclass labelPickListenerClass = jniEnv->FindClass("com/mapzen/tangram/MapController$LabelPickListener");
    onLabelPickMID = jniEnv->GetMethodID(labelPickListenerClass, "onLabelPick", "(Lcom/mapzen/tangram/LabelPickResult;FF)V");

    if (labelPickResultClass) {
        jniEnv->DeleteGlobalRef(labelPickResultClass);
    }
    labelPickResultClass = (jclass)jniEnv->NewGlobalRef(jniEnv->FindClass("com/mapzen/tangram/LabelPickResult"));
    labelPickResultInitMID = jniEnv->GetMethodID(labelPickResultClass, "<init>", "(DDILjava/util/Map;)V");

    if (markerPickResultClass) {
        jniEnv->DeleteGlobalRef(markerPickResultClass);
    }
    markerPickResultClass = (jclass)jniEnv->NewGlobalRef(jniEnv->FindClass("com/mapzen/tangram/MarkerPickResult"));
    markerPickResultInitMID = jniEnv->GetMethodID(markerPickResultClass, "<init>", "(Lcom/mapzen/tangram/Marker;DD)V");

    if (sceneErrorClass) {
        jniEnv->DeleteGlobalRef(sceneErrorClass);
    }
    sceneErrorClass = (jclass)jniEnv->NewGlobalRef(jniEnv->FindClass("com/mapzen/tangram/SceneError"));
    sceneErrorInitMID = jniEnv->GetMethodID(sceneErrorClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;I)V");

    if (hashmapClass) {
        jniEnv->DeleteGlobalRef(hashmapClass);
    }
    hashmapClass = (jclass)jniEnv->NewGlobalRef(jniEnv->FindClass("java/util/HashMap"));
    hashmapInitMID = jniEnv->GetMethodID(hashmapClass, "<init>", "()V");
    hashmapPutMID = jniEnv->GetMethodID(hashmapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    markerByIDMID = jniEnv->GetMethodID(tangramClass, "markerById", "(J)Lcom/mapzen/tangram/Marker;");

    jclass markerPickListenerClass = jniEnv->FindClass("com/mapzen/tangram/MapController$MarkerPickListener");
    onMarkerPickMID = jniEnv->GetMethodID(markerPickListenerClass, "onMarkerPick", "(Lcom/mapzen/tangram/MarkerPickResult;FF)V");
}

std::string stringFromJString(JNIEnv* jniEnv, jstring string) {
    auto length = jniEnv->GetStringLength(string);
    std::u16string chars(length, char16_t());
    if(!chars.empty()) {
        jniEnv->GetStringRegion(string, 0, length, reinterpret_cast<jchar*>(&chars[0]));
    }
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes(chars);
}

jstring jstringFromString(JNIEnv* jniEnv, const std::string& string) {
    const auto emptyu16 = u"";
    auto chars = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().from_bytes(string);
    auto s = reinterpret_cast<const jchar*>(chars.empty() ? emptyu16 : chars.data());
    return jniEnv->NewString(s, chars.length());
}

class JniThreadBinding {
private:
    JavaVM* jvm;
    JNIEnv *jniEnv;
    int status;
public:
    JniThreadBinding(JavaVM* _jvm) : jvm(_jvm) {
        status = jvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) { jvm->AttachCurrentThread(&jniEnv, NULL);}
    }
    ~JniThreadBinding() {
        if (status == JNI_EDETACHED) { jvm->DetachCurrentThread(); }
    }

    JNIEnv* operator->() const {
        return jniEnv;
    }

    operator JNIEnv*() const {
        return jniEnv;
    }
};

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
    va_end(args);

}

std::string AndroidPlatform::fontPath(const std::string& _family, const std::string& _weight, const std::string& _style) const {

    JniThreadBinding jniEnv(jvm);

    std::string key = _family + "_" + _weight + "_" + _style;

    jstring jkey = jstringFromString(jniEnv, key);
    jstring returnStr = (jstring) jniEnv->CallObjectMethod(m_tangramInstance, getFontFilePath, jkey);

    auto resultStr = stringFromJString(jniEnv, returnStr);
    jniEnv->DeleteLocalRef(returnStr);
    jniEnv->DeleteLocalRef(jkey);

    return resultStr;
}

AndroidPlatform::AndroidPlatform(JNIEnv* _jniEnv, jobject _assetManager, jobject _tangramInstance) {
    m_tangramInstance = _jniEnv->NewGlobalRef(_tangramInstance);

    m_assetManager = AAssetManager_fromJava(_jniEnv, _assetManager);

    if (m_assetManager == nullptr) {
        LOGE("Could not obtain Asset Manager reference");
        return;
    }

    sqlite3_ndk_init(m_assetManager);
}

void AndroidPlatform::dispose(JNIEnv* _jniEnv) {
    _jniEnv->DeleteGlobalRef(m_tangramInstance);
}

void AndroidPlatform::requestRender() const {

    JniThreadBinding jniEnv(jvm);

    jniEnv->CallVoidMethod(m_tangramInstance, requestRenderMethodID);
}

std::string AndroidPlatform::fontFallbackPath(int _importance, int _weightHint) const {

    JniThreadBinding jniEnv(jvm);

    jstring returnStr = (jstring) jniEnv->CallObjectMethod(m_tangramInstance, getFontFallbackFilePath, _importance, _weightHint);

    auto resultStr = stringFromJString(jniEnv, returnStr);
    jniEnv->DeleteLocalRef(returnStr);

    return resultStr;
}

std::vector<FontSourceHandle> AndroidPlatform::systemFontFallbacksHandle() const {
    std::vector<FontSourceHandle> handles;

    int importance = 0;
    int weightHint = 400;

    std::string fallbackPath = fontFallbackPath(importance, weightHint);

    while (!fallbackPath.empty()) {
        handles.emplace_back(Url(fallbackPath));

        fallbackPath = fontFallbackPath(importance++, weightHint);
    }

    return handles;
}

FontSourceHandle AndroidPlatform::systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const {
    std::string path = fontPath(_name, _weight, _face);

    if (path.empty()) { return {}; }

    auto data = bytesFromFile(path.c_str());

    return FontSourceHandle([data]() { return data; });
}

void AndroidPlatform::setContinuousRendering(bool _isContinuous) {
    Platform::setContinuousRendering(_isContinuous);

    JniThreadBinding jniEnv(jvm);

    jniEnv->CallVoidMethod(m_tangramInstance, setRenderModeMethodID, _isContinuous ? 1 : 0);
}

bool AndroidPlatform::bytesFromAssetManager(const char* _path, std::function<char*(size_t)> _allocator) const {

    AAsset* asset = AAssetManager_open(m_assetManager, _path, AASSET_MODE_UNKNOWN);
    if (asset == nullptr) {
        LOGW("Failed to open asset at path: %s", _path);
        return false;
    }

    size_t size = AAsset_getLength(asset);
    unsigned char* data = reinterpret_cast<unsigned char*>(_allocator(size));

    int read = AAsset_read(asset, data, size);
    if (read <= 0) {
        LOGW("Failed to read asset at path: %s", _path);
    }
    AAsset_close(asset);

    return read > 0;
}

std::vector<char> AndroidPlatform::bytesFromFile(const Url& url) const {
    std::vector<char> data;

    auto allocator = [&](size_t size) {
        data.resize(size);
        return data.data();
    };

    auto path = url.path();

    if (url.scheme() == "asset") {
        // The asset manager doesn't like paths starting with '/'.
        if (!path.empty() && path.front() == '/') {
            path = path.substr(1);
        }
        bytesFromAssetManager(path.c_str(), allocator);
    } else {
        Platform::bytesFromFileSystem(path.c_str(), allocator);
    }

    return data;
}

UrlRequestHandle AndroidPlatform::startUrlRequest(Url _url, UrlCallback _callback) {

    JniThreadBinding jniEnv(jvm);

    // Get the current value of the request counter and add one, atomically.
    UrlRequestHandle requestHandle = m_urlRequestCount++;

    // If the requested URL does not use HTTP or HTTPS, retrieve it synchronously.
    if (!_url.hasHttpScheme()) {
        UrlResponse response;
        response.content = bytesFromFile(_url);
        if (_callback) {
            _callback(response);
        }
        return requestHandle;
    }

    // Store our callback, associated with the request handle.
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_callbacks[requestHandle] = _callback;
    }

    jlong jRequestHandle = static_cast<jlong>(requestHandle);

    // Check that it's safe to convert the UrlRequestHandle to a jlong and back.
    assert(requestHandle == static_cast<UrlRequestHandle>(jRequestHandle));

    jstring jUrl = jstringFromString(jniEnv, _url.string());

    // Call the MapController method to start the URL request.
    jniEnv->CallVoidMethod(m_tangramInstance, startUrlRequestMID, jUrl, jRequestHandle);

    return requestHandle;
}

void AndroidPlatform::cancelUrlRequest(UrlRequestHandle request) {

    JniThreadBinding jniEnv(jvm);

    jlong jRequestHandle = static_cast<jlong>(request);

    jniEnv->CallVoidMethod(m_tangramInstance, cancelUrlRequestMID, jRequestHandle);

    // We currently don't try to cancel requests for local files.
}

void AndroidPlatform::onUrlComplete(JNIEnv* _jniEnv, jlong _jRequestHandle, jbyteArray _jBytes, jstring _jError) {
    // Start building a response object.
    UrlResponse response;

    // If the request was successful, we will receive a non-null byte array.
    if (_jBytes != nullptr) {
        size_t length = _jniEnv->GetArrayLength(_jBytes);
        response.content.resize(length);
        _jniEnv->GetByteArrayRegion(_jBytes, 0, length, reinterpret_cast<jbyte*>(response.content.data()));
        // TODO: Can we use a DirectByteBuffer to transfer data with fewer copies?
    }

    // If the request was unsuccessful, we will receive a non-null error string.
    std::string error;
    if (_jError != nullptr) {
        error = stringFromJString(_jniEnv, _jError);
        response.error = error.c_str();
    }

    // Find the callback associated with the request.
    UrlCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        UrlRequestHandle requestHandle = static_cast<UrlRequestHandle>(_jRequestHandle);
        auto it = m_callbacks.find(requestHandle);
        if (it != m_callbacks.end()) {
            callback = std::move(it->second);
            m_callbacks.erase(it);
        }
    }
    if (callback) {
        callback(response);
    }
}

void setCurrentThreadPriority(int priority) {
    int  tid = gettid();
    setpriority(PRIO_PROCESS, tid, priority);
}

void labelPickCallback(jobject listener, const Tangram::LabelPickResult* labelPickResult) {

    JniThreadBinding jniEnv(jvm);

    float position[2] = {0.0, 0.0};

    jobject labelPickResultObject = nullptr;

    if (labelPickResult) {
        auto properties = labelPickResult->touchItem.properties;

        position[0] = labelPickResult->touchItem.position[0];
        position[1] = labelPickResult->touchItem.position[1];

        jobject hashmap = jniEnv->NewObject(hashmapClass, hashmapInitMID);

        for (const auto& item : properties->items()) {
            jstring jkey = jstringFromString(jniEnv, item.key);
            jstring jvalue = jstringFromString(jniEnv, properties->asString(item.value));
            jniEnv->CallObjectMethod(hashmap, hashmapPutMID, jkey, jvalue);
        }

        labelPickResultObject = jniEnv->NewObject(labelPickResultClass, labelPickResultInitMID, labelPickResult->coordinates.longitude,
            labelPickResult->coordinates.latitude, labelPickResult->type, hashmap);
    }

    jniEnv->CallVoidMethod(listener, onLabelPickMID, labelPickResultObject, position[0], position[1]);
    jniEnv->DeleteGlobalRef(listener);
}

void markerPickCallback(jobject listener, jobject tangramInstance, const Tangram::MarkerPickResult* markerPickResult) {

    JniThreadBinding jniEnv(jvm);
    float position[2] = {0.0, 0.0};

    jobject markerPickResultObject = nullptr;

    if (markerPickResult) {
        jobject marker = nullptr;

        position[0] = markerPickResult->position[0];
        position[1] = markerPickResult->position[1];

        marker = jniEnv->CallObjectMethod(tangramInstance, markerByIDMID, static_cast<jlong>(markerPickResult->id));

        if (marker) {
            markerPickResultObject = jniEnv->NewObject(markerPickResultClass,
                                                       markerPickResultInitMID, marker,
                                                       markerPickResult->coordinates.longitude,
                                                       markerPickResult->coordinates.latitude);
        }
    }

    jniEnv->CallVoidMethod(listener, onMarkerPickMID, markerPickResultObject, position[0], position[1]);
    jniEnv->DeleteGlobalRef(listener);
    jniEnv->DeleteGlobalRef(tangramInstance);
}

void featurePickCallback(jobject listener, const Tangram::FeaturePickResult* featurePickResult) {

    JniThreadBinding jniEnv(jvm);

    jobject hashmap = jniEnv->NewObject(hashmapClass, hashmapInitMID);
    float position[2] = {0.0, 0.0};

    if (featurePickResult) {
        auto properties = featurePickResult->properties;

        position[0] = featurePickResult->position[0];
        position[1] = featurePickResult->position[1];

        for (const auto& item : properties->items()) {
            jstring jkey = jstringFromString(jniEnv, item.key);
            jstring jvalue = jstringFromString(jniEnv, properties->asString(item.value));
            jniEnv->CallObjectMethod(hashmap, hashmapPutMID, jkey, jvalue);
        }
    }

    jniEnv->CallVoidMethod(listener, onFeaturePickMID, hashmap, position[0], position[1]);
    jniEnv->DeleteGlobalRef(listener);
}

void initGLExtensions() {
    if (glExtensionsLoaded) {
        return;
    }

    void* libhandle = dlopen("libGLESv2.so", RTLD_LAZY);

    glBindVertexArrayOESEXT = (PFNGLBINDVERTEXARRAYOESPROC) dlsym(libhandle, "glBindVertexArrayOES");
    glDeleteVertexArraysOESEXT = (PFNGLDELETEVERTEXARRAYSOESPROC) dlsym(libhandle, "glDeleteVertexArraysOES");
    glGenVertexArraysOESEXT = (PFNGLGENVERTEXARRAYSOESPROC) dlsym(libhandle, "glGenVertexArraysOES");

    glExtensionsLoaded = true;
}

void AndroidPlatform::sceneReadyCallback(SceneID id, const SceneError* sceneError) {

    JniThreadBinding jniEnv(jvm);

    jobject jUpdateErrorStatus = 0;

    if (sceneError) {
        jstring jUpdateStatusPath = jstringFromString(jniEnv, sceneError->update.path);
        jstring jUpdateStatusValue = jstringFromString(jniEnv, sceneError->update.value);
        jint jError = (jint) sceneError->error;
        jUpdateErrorStatus = jniEnv->NewObject(sceneErrorClass,
                                               sceneErrorInitMID,
                                               jUpdateStatusPath, jUpdateStatusValue,
                                               jError);
    }

    jniEnv->CallVoidMethod(m_tangramInstance, sceneReadyCallbackMID, id, jUpdateErrorStatus);
}

} // namespace Tangram
