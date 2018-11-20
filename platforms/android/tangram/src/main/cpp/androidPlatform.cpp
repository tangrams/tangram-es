#include "androidPlatform.h"

#include "jniThreadBinding.h"

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

#ifdef TANGRAM_MBTILES_DATASOURCE
#include "sqlite3ndk.h"
#endif

PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT = 0;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT = 0;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT = 0;

#define TANGRAM_JNI_VERSION JNI_VERSION_1_6

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
static jmethodID featurePickCallbackMID = 0;
static jmethodID labelPickCallbackMID = 0;
static jmethodID markerPickCallbackMID = 0;
static jmethodID sceneReadyCallbackMID = 0;
static jmethodID cameraAnimationCallbackMID = 0;

static jclass hashmapClass = nullptr;
static jmethodID hashmapInitMID = 0;
static jmethodID hashmapPutMID = 0;

static bool glExtensionsLoaded = false;


void AndroidPlatform::bindJniEnvToThread(JNIEnv* jniEnv) {
    jniEnv->GetJavaVM(&jvm);
}

jint AndroidPlatform::jniOnLoad(JavaVM* javaVM) {
    // JNI OnLoad is invoked once when the native library is loaded so this is a good place to cache
    // any method or class IDs that we'll need.

    jvm = javaVM;
    JNIEnv* jniEnv = nullptr;
    if (javaVM->GetEnv(reinterpret_cast<void**>(&jniEnv), TANGRAM_JNI_VERSION) != JNI_OK) {
        return -1;
    }

    jclass tangramClass = jniEnv->FindClass("com/mapzen/tangram/MapController");
    startUrlRequestMID = jniEnv->GetMethodID(tangramClass, "startUrlRequest", "(Ljava/lang/String;J)V");
    cancelUrlRequestMID = jniEnv->GetMethodID(tangramClass, "cancelUrlRequest", "(J)V");
    getFontFilePath = jniEnv->GetMethodID(tangramClass, "getFontFilePath", "(Ljava/lang/String;)Ljava/lang/String;");
    getFontFallbackFilePath = jniEnv->GetMethodID(tangramClass, "getFontFallbackFilePath", "(II)Ljava/lang/String;");
    requestRenderMethodID = jniEnv->GetMethodID(tangramClass, "requestRender", "()V");
    setRenderModeMethodID = jniEnv->GetMethodID(tangramClass, "setRenderMode", "(I)V");
    sceneReadyCallbackMID = jniEnv->GetMethodID(tangramClass, "sceneReadyCallback", "(IILjava/lang/String;Ljava/lang/String;)V");
    cameraAnimationCallbackMID = jniEnv->GetMethodID(tangramClass, "cameraAnimationCallback", "(Z)V");
    featurePickCallbackMID = jniEnv->GetMethodID(tangramClass, "featurePickCallback", "(Ljava/util/Map;FF)V");
    labelPickCallbackMID = jniEnv->GetMethodID(tangramClass, "labelPickCallback", "(Ljava/util/Map;FFIDD)V");
    markerPickCallbackMID = jniEnv->GetMethodID(tangramClass, "markerPickCallback", "(JFFDD)V");

    // We need a reference to the class object later to invoke the constructor. FindClass produces a
    // local reference that may not be valid later, so create a global reference to the class.
    hashmapClass = (jclass)jniEnv->NewGlobalRef(jniEnv->FindClass("java/util/HashMap"));
    hashmapInitMID = jniEnv->GetMethodID(hashmapClass, "<init>", "()V");
    hashmapPutMID = jniEnv->GetMethodID(hashmapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    return TANGRAM_JNI_VERSION;
}

void AndroidPlatform::jniOnUnload(JavaVM *javaVM) {
    JNIEnv* jniEnv = nullptr;
    if (javaVM->GetEnv(reinterpret_cast<void**>(&jniEnv), TANGRAM_JNI_VERSION) != JNI_OK) {
        return;
    }
    jniEnv->DeleteGlobalRef(hashmapClass);
    hashmapClass = nullptr;

    jvm = nullptr;
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

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
    va_end(args);

}


AndroidPlatform::AndroidPlatform(JNIEnv* _jniEnv, jobject _assetManager, jobject _tangramInstance)
    : m_jniWorker(jvm) {

    m_tangramInstance = _jniEnv->NewWeakGlobalRef(_tangramInstance);

    m_assetManager = AAssetManager_fromJava(_jniEnv, _assetManager);

    if (m_assetManager == nullptr) {
        LOGE("Could not obtain Asset Manager reference");
        return;
    }

#ifdef TANGRAM_MBTILES_DATASOURCE
    sqlite3_ndk_init(m_assetManager);
#endif
}

void AndroidPlatform::shutdown() {
    Platform::shutdown();
    m_jniWorker.stop();
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

void AndroidPlatform::requestRender() const {
    m_jniWorker.enqueue([&](JNIEnv *jniEnv) {
        jniEnv->CallVoidMethod(m_tangramInstance, requestRenderMethodID);
    });
}

std::vector<FontSourceHandle> AndroidPlatform::systemFontFallbacksHandle() const {
    JniThreadBinding jniEnv(jvm);

    std::vector<FontSourceHandle> handles;

    int importance = 0;
    int weightHint = 400;

    auto fontFallbackPath = [&](int _importance, int _weightHint) {

        jstring returnStr = (jstring) jniEnv->CallObjectMethod(m_tangramInstance,
                                                               getFontFallbackFilePath, _importance,
                                                               _weightHint);

        auto resultStr = stringFromJString(jniEnv, returnStr);
        jniEnv->DeleteLocalRef(returnStr);

        return resultStr;
    };

    std::string fallbackPath = fontFallbackPath(importance, weightHint);

    while (!fallbackPath.empty()) {
        handles.emplace_back(Url(fallbackPath));

        fallbackPath = fontFallbackPath(++importance, weightHint);
    }

    return handles;
}

FontSourceHandle AndroidPlatform::systemFont(const std::string& _name, const std::string& _weight,
                                             const std::string& _face) const {
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

bool AndroidPlatform::startUrlRequestImpl(const Url& _url, const UrlRequestHandle _request, UrlRequestId& _id) {

    // If the requested URL does not use HTTP or HTTPS, retrieve it asynchronously.
    if (!_url.hasHttpScheme()) {
        m_fileWorker.enqueue([=](){
             UrlResponse response;
             response.content = bytesFromFile(_url);
             onUrlResponse(_request, std::move(response));
        });
        return false;
    }

    // We can use UrlRequestHandle to cancel requests. MapController handles the
    // mapping between UrlRequestHandle and request object
    _id = _request;

    m_jniWorker.enqueue([=](JNIEnv *jniEnv) {
        jlong jRequestHandle = static_cast<jlong>(_request);

        // Make sure no one changed UrlRequestHandle from being uint64_t,
        // so that it's safe to convert to jlong and back.
        static_assert(sizeof(jlong) == sizeof(UrlRequestHandle), "Who changed UrlRequestHandle?!");
        static_assert(static_cast<jlong>(std::numeric_limits<uint64_t>::max()) ==
                      static_cast<UrlRequestHandle>(std::numeric_limits<uint64_t>::max()),
                      "Cannot convert jlong to UrlRequestHandle!");

        jstring jUrl = jstringFromString(jniEnv, _url.string());

        // Call the MapController method to start the URL request.
        jniEnv->CallVoidMethod(m_tangramInstance, startUrlRequestMID, jUrl, jRequestHandle);
    });

    return true;
}

void AndroidPlatform::cancelUrlRequestImpl(const UrlRequestId _id) {

    m_jniWorker.enqueue([=](JNIEnv *jniEnv) {

        jlong jRequestHandle = static_cast<jlong>(_id);

        jniEnv->CallVoidMethod(m_tangramInstance, cancelUrlRequestMID, jRequestHandle);
    });
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

    // Handle callbacks on worker thread to not block Java side.
    // (The calling thread has probably also other work to do)
    m_fileWorker.enqueue([this, _jRequestHandle, r = std::move(response)]() mutable {
        UrlRequestHandle requestHandle = static_cast<UrlRequestHandle>(_jRequestHandle);

        onUrlResponse(requestHandle, std::move(r));
    });
}

void setCurrentThreadPriority(int priority) {
    setpriority(PRIO_PROCESS, 0, priority);
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

AndroidMap::AndroidMap(JNIEnv* _jniEnv, jobject _assetManager, jobject _tangramInstance)
        : Map(std::make_unique<AndroidPlatform>(_jniEnv, _assetManager, _tangramInstance)) {

    m_tangramInstance = _jniEnv->NewWeakGlobalRef(_tangramInstance);

    setSceneReadyListener([this](Tangram::SceneID id, const Tangram::SceneError* sceneError) {
        JniThreadBinding jniEnv(jvm);

        jint jErrorType = -1;
        jstring jUpdatePath = nullptr;
        jstring jUpdateValue = nullptr;

        if (sceneError) {
            jUpdatePath = jstringFromString(jniEnv, sceneError->update.path);
            jUpdateValue = jstringFromString(jniEnv, sceneError->update.value);
            jErrorType = (jint)sceneError->error;
        }

        jniEnv->CallVoidMethod(m_tangramInstance, sceneReadyCallbackMID, id, jErrorType, jUpdatePath, jUpdateValue);
    });

    setCameraAnimationListener([this](bool finished) {
        JniThreadBinding jniEnv(jvm);

        jniEnv->CallVoidMethod(m_tangramInstance, cameraAnimationCallbackMID, finished);
    });
}

void AndroidMap::pickFeature(float posX, float posY) {

    pickFeatureAt(posX, posY, [this](auto featurePickResult) {
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
        jniEnv->CallVoidMethod(m_tangramInstance, featurePickCallbackMID, hashmap, position[0], position[1]);
    });
}

void AndroidMap::pickLabel(float posX, float posY) {

    pickLabelAt(posX, posY, [this](auto labelPickResult){
        JniThreadBinding jniEnv(jvm);

        float x = 0.f, y = 0.f;
        double lng = 0., lat = 0.;
        int type = 0;
        jobject hashmap = nullptr;

        if (labelPickResult) {
            auto properties = labelPickResult->touchItem.properties;

            x = labelPickResult->touchItem.position[0];
            y = labelPickResult->touchItem.position[1];

            hashmap = jniEnv->NewObject(hashmapClass, hashmapInitMID);

            for (const auto& item : properties->items()) {
                jstring jkey = jstringFromString(jniEnv, item.key);
                jstring jvalue = jstringFromString(jniEnv, properties->asString(item.value));
                jniEnv->CallObjectMethod(hashmap, hashmapPutMID, jkey, jvalue);
            }
        }
        jniEnv->CallVoidMethod(m_tangramInstance, labelPickCallbackMID, hashmap, x, y, lng, lat, type);
    });
}

void AndroidMap::pickMarker(float posX, float posY) {

    pickMarkerAt(posX, posY, [this](auto markerPickResult) {
        JniThreadBinding jniEnv(jvm);

        float x = 0.f, y = 0.f;
        double lng = 0., lat = 0.;
        long markerID = 0;

        if (markerPickResult) {
            x = markerPickResult->position[0];
            y = markerPickResult->position[1];
            lng = markerPickResult->coordinates.longitude;
            lat = markerPickResult->coordinates.latitude;
            markerID = static_cast<long>(markerPickResult->id);
        }

        jniEnv->CallVoidMethod(m_tangramInstance, markerPickCallbackMID, markerID, x, y, lng, lat);
    });
}

} // namespace Tangram
