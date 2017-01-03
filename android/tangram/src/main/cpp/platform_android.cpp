#ifdef PLATFORM_ANDROID

#include "platform_android.h"
#include "data/properties.h"
#include "data/propertyItem.h"
#include "util/url.h"
#include "tangram.h"

#include <GLES2/gl2platform.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h> // dlopen, dlsym

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <cstdarg>

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <fstream>
#include <algorithm>

#include <regex>

#include "log.h"

/* Followed the following document for JavaVM tips when used with native threads
 * http://android.wooyd.org/JNIExample/#NWD1sCYeT-I
 * http://developer.android.com/training/articles/perf-jni.html and
 * http://www.ibm.com/developerworks/library/j-jni/
 * http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/invocation.html
 */

static JavaVM* jvm = nullptr;
// JNI Env bound on androids render thread (our native main thread)
static jobject tangramInstance = nullptr;
static jmethodID requestRenderMethodID = 0;
static jmethodID setRenderModeMethodID = 0;
static jmethodID startUrlRequestMID = 0;
static jmethodID cancelUrlRequestMID = 0;
static jmethodID getFontFilePath = 0;
static jmethodID getFontFallbackFilePath = 0;
static jmethodID onFeaturePickMID = 0;
static jmethodID onLabelPickMID = 0;
static jmethodID labelPickResultInitMID = 0;

static jclass labelPickResultClass = nullptr;
static jclass hashmapClass = nullptr;
static jmethodID hashmapInitMID = 0;
static jmethodID hashmapPutMID = 0;

static jclass markerClass = nullptr;
static jmethodID markerByIDMID = 0;

static AAssetManager* assetManager = nullptr;

static bool s_isContinuousRendering = false;

PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT = 0;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT = 0;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT = 0;

// Android assets are distinguished from file paths by the "asset" scheme.
static const char* aaPrefix = "asset:///";
static const size_t aaPrefixLen = 9;

void bindJniEnvToThread(JNIEnv* jniEnv) {
    jniEnv->GetJavaVM(&jvm);
}

void setupJniEnv(JNIEnv* jniEnv, jobject _tangramInstance, jobject _assetManager) {
    bindJniEnvToThread(jniEnv);
    if (tangramInstance) {
        jniEnv->DeleteGlobalRef(tangramInstance);
    }
    tangramInstance = jniEnv->NewGlobalRef(_tangramInstance);
    jclass tangramClass = jniEnv->FindClass("com/mapzen/tangram/MapController");
    startUrlRequestMID = jniEnv->GetMethodID(tangramClass, "startUrlRequest", "(Ljava/lang/String;J)Z");
    cancelUrlRequestMID = jniEnv->GetMethodID(tangramClass, "cancelUrlRequest", "(Ljava/lang/String;)V");
    getFontFilePath = jniEnv->GetMethodID(tangramClass, "getFontFilePath", "(Ljava/lang/String;)Ljava/lang/String;");
    getFontFallbackFilePath = jniEnv->GetMethodID(tangramClass, "getFontFallbackFilePath", "(II)Ljava/lang/String;");
    requestRenderMethodID = jniEnv->GetMethodID(tangramClass, "requestRender", "()V");
    setRenderModeMethodID = jniEnv->GetMethodID(tangramClass, "setRenderMode", "(I)V");

    jclass featurePickListenerClass = jniEnv->FindClass("com/mapzen/tangram/MapController$FeaturePickListener");
    onFeaturePickMID = jniEnv->GetMethodID(featurePickListenerClass, "onFeaturePick", "(Ljava/util/Map;FF)V");
    jclass labelPickListenerClass = jniEnv->FindClass("com/mapzen/tangram/MapController$LabelPickListener");
    onLabelPickMID = jniEnv->GetMethodID(labelPickListenerClass, "onLabelPick", "(Lcom/mapzen/tangram/LabelPickResult;FF)V");

    if (labelPickResultClass) {
        jniEnv->DeleteGlobalRef(labelPickResultClass);
    }
    labelPickResultClass = (jclass)jniEnv->NewGlobalRef(jniEnv->FindClass("com/mapzen/tangram/LabelPickResult"));
    labelPickResultInitMID = jniEnv->GetMethodID(labelPickResultClass, "<init>", "(DDILjava/util/Map;)V");

    if (hashmapClass) {
        jniEnv->DeleteGlobalRef(hashmapClass);
    }
    hashmapClass = (jclass)jniEnv->NewGlobalRef(jniEnv->FindClass("java/util/HashMap"));
    hashmapInitMID = jniEnv->GetMethodID(hashmapClass, "<init>", "()V");
    hashmapPutMID = jniEnv->GetMethodID(hashmapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    markerClass = jniEnv->FindClass("com/mapzen/tangram/Marker");
    markerByIDMID = jniEnv->GetMethodID(tangramClass, "markerById", "(J)Lcom/mapzen/tangram/Marker;");

    assetManager = AAssetManager_fromJava(jniEnv, _assetManager);

    if (assetManager == nullptr) {
        logMsg("ERROR: Could not obtain Asset Manager reference\n");
    }

}

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
    va_end(args);

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

void requestRender() {

    JniThreadBinding jniEnv(jvm);

    jniEnv->CallVoidMethod(tangramInstance, requestRenderMethodID);
}

std::string fontFallbackPath(int _importance, int _weightHint) {

    JniThreadBinding jniEnv(jvm);

    jstring returnStr = (jstring) jniEnv->CallObjectMethod(tangramInstance, getFontFallbackFilePath, _importance, _weightHint);

    auto resultStr = stringFromJString(jniEnv, returnStr);
    jniEnv->DeleteLocalRef(returnStr);

    return resultStr;
}

std::string fontPath(const std::string& _family, const std::string& _weight, const std::string& _style) {

    JniThreadBinding jniEnv(jvm);

    std::string key = _family + "_" + _weight + "_" + _style;

    jstring jkey = jniEnv->NewStringUTF(key.c_str());
    jstring returnStr = (jstring) jniEnv->CallObjectMethod(tangramInstance, getFontFilePath, jkey);

    auto resultStr = stringFromJString(jniEnv, returnStr);
    jniEnv->DeleteLocalRef(returnStr);
    jniEnv->DeleteLocalRef(jkey);

    return resultStr;
}

std::vector<FontSourceHandle> systemFontFallbacksHandle() {
    std::vector<FontSourceHandle> handles;

    int importance = 0;
    int weightHint = 400;

    std::string fallbackPath = fontFallbackPath(importance, weightHint);

    while (!fallbackPath.empty()) {
        LOG("Loading font %s", fallbackPath.c_str());

        handles.emplace_back(fallbackPath);

        fallbackPath = fontFallbackPath(importance++, weightHint);
    }

    return handles;
}

unsigned char* systemFont(const std::string& _name, const std::string& _weight, const std::string& _face, size_t* _size) {
    std::string path = fontPath(_name, _weight, _face);

    if (path.empty()) { return nullptr; }

    return bytesFromFile(path.c_str(), *_size);
}

void setContinuousRendering(bool _isContinuous) {

    s_isContinuousRendering = _isContinuous;

    JniThreadBinding jniEnv(jvm);

    jniEnv->CallVoidMethod(tangramInstance, setRenderModeMethodID, _isContinuous ? 1 : 0);
}

bool isContinuousRendering() {

    return s_isContinuousRendering;

}

bool bytesFromAssetManager(const char* _path, std::function<char*(size_t)> _allocator) {

    AAsset* asset = AAssetManager_open(assetManager, _path, AASSET_MODE_UNKNOWN);
    if (asset == nullptr) {
        logMsg("Failed to open asset at path: %s\n", _path);
        return false;
    }

    size_t size = AAsset_getLength(asset);
    unsigned char* data = reinterpret_cast<unsigned char*>(_allocator(size));

    int read = AAsset_read(asset, data, size);
    if (read <= 0) {
        logMsg("Failed to read asset at path: %s\n", _path);
    }
    AAsset_close(asset);

    return read > 0;
}

bool bytesFromFileSystem(const char* _path, std::function<char*(size_t)> _allocator) {

    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        return false;
    }

    size_t size = resource.tellg();
    char* cdata = _allocator(size);

    resource.seekg(std::ifstream::beg);
    resource.read(cdata, size);
    resource.close();

    return true;
}

std::string stringFromFile(const char* _path) {

    std::string data;

    auto allocator = [&](size_t size) {
        data.resize(size);
        return &data[0];
    };

    if (strncmp(_path, aaPrefix, aaPrefixLen) == 0) {
        bytesFromAssetManager(_path + aaPrefixLen, allocator);
    } else {
        bytesFromFileSystem(_path, allocator);
    }
    return data;
}

unsigned char* bytesFromFile(const char* _path, size_t& _size) {

    _size = 0;
    unsigned char* data = nullptr;

    auto allocator = [&](size_t size) {
        _size = size;
        data = (unsigned char*) malloc(sizeof(char) * size);
        return reinterpret_cast<char*>(data);
    };

    if (strncmp(_path, aaPrefix, aaPrefixLen) == 0) {
        bytesFromAssetManager(_path + aaPrefixLen, allocator);
    } else {
        bytesFromFileSystem(_path, allocator);
    }
    return data;
}

std::string resolveScenePath(const char* path) {
    // If the path is an absolute URL (like a file:// or http:// URL)
    // then resolving it will return the same URL. Otherwise, we resolve
    // it against the "asset" scheme to know later that this path is in
    // the asset bundle.
    return Tangram::Url(path).resolved("asset:///").string();
}

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {

    JniThreadBinding jniEnv(jvm);
    jstring jUrl = jniEnv->NewStringUTF(_url.c_str());

    // This is probably super dangerous. In order to pass a reference to our callback we have to convert it
    // to a Java type. We allocate a new callback object and then reinterpret the pointer to it as a Java long.
    // In Java, we associate this long with the current network request and pass it back to native code when
    // the request completes (either in onUrlSuccess or onUrlFailure), reinterpret the long back into a
    // pointer, call the callback function if the request succeeded, and delete the heap-allocated UrlCallback
    // to make sure nothing is leaked.
    jlong jCallbackPtr = reinterpret_cast<jlong>(new UrlCallback(_callback));

    jboolean methodResult = jniEnv->CallBooleanMethod(tangramInstance, startUrlRequestMID, jUrl, jCallbackPtr);

    return methodResult;
}

void cancelUrlRequest(const std::string& _url) {
    JniThreadBinding jniEnv(jvm);
    jstring jUrl = jniEnv->NewStringUTF(_url.c_str());
    jniEnv->CallVoidMethod(tangramInstance, cancelUrlRequestMID, jUrl);
}

void onUrlSuccess(JNIEnv* _jniEnv, jbyteArray _jBytes, jlong _jCallbackPtr) {

    size_t length = _jniEnv->GetArrayLength(_jBytes);
    std::vector<char> content;
    content.resize(length);

    _jniEnv->GetByteArrayRegion(_jBytes, 0, length, reinterpret_cast<jbyte*>(content.data()));

    UrlCallback* callback = reinterpret_cast<UrlCallback*>(_jCallbackPtr);
    (*callback)(std::move(content));
    delete callback;
}

void onUrlFailure(JNIEnv* _jniEnv, jlong _jCallbackPtr) {
    std::vector<char> empty;

    UrlCallback* callback = reinterpret_cast<UrlCallback*>(_jCallbackPtr);
    (*callback)(std::move(empty));
    delete callback;
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
            jstring jkey = jniEnv->NewStringUTF(item.key.c_str());
            jstring jvalue = jniEnv->NewStringUTF(properties->asString(item.value).c_str());
            jniEnv->CallObjectMethod(hashmap, hashmapPutMID, jkey, jvalue);
        }

        jdoubleArray darr = jniEnv->NewDoubleArray(2);
        labelPickResultObject = jniEnv->NewObject(labelPickResultClass, labelPickResultInitMID, labelPickResult->coordinates.longitude,
            labelPickResult->coordinates.latitude, labelPickResult->type, hashmap);
    }

    jniEnv->CallVoidMethod(listener, onLabelPickMID, labelPickResultObject, position[0], position[1]);
    jniEnv->DeleteGlobalRef(listener);
}

void markerPickCallback(jobject listener, const Tangram::MarkerPickResult* markerPickResult) {

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
            jstring jkey = jniEnv->NewStringUTF(item.key.c_str());
            jstring jvalue = jniEnv->NewStringUTF(properties->asString(item.value).c_str());
            jniEnv->CallObjectMethod(hashmap, hashmapPutMID, jkey, jvalue);
        }
    }

    jniEnv->CallVoidMethod(listener, onFeaturePickMID, hashmap, position[0], position[1]);
    jniEnv->DeleteGlobalRef(listener);
}


void initGLExtensions() {
    void* libhandle = dlopen("libGLESv2.so", RTLD_LAZY);

    glBindVertexArrayOESEXT = (PFNGLBINDVERTEXARRAYOESPROC) dlsym(libhandle, "glBindVertexArrayOES");
    glDeleteVertexArraysOESEXT = (PFNGLDELETEVERTEXARRAYSOESPROC) dlsym(libhandle, "glDeleteVertexArraysOES");
    glGenVertexArraysOESEXT = (PFNGLGENVERTEXARRAYSOESPROC) dlsym(libhandle, "glGenVertexArraysOES");
}

std::string stringFromJString(JNIEnv* jniEnv, jstring string) {
    size_t length = jniEnv->GetStringLength(string);
    std::string out(length, 0);
    jniEnv->GetStringUTFRegion(string, 0, length, &out[0]);
    return out;
}

#endif
