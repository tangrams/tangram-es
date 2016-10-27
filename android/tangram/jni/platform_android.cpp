#ifdef PLATFORM_ANDROID

#include "platform_android.h"
#include "data/properties.h"
#include "data/propertyItem.h"
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

/* Followed the following document for JavaVM tips when used with native threads
 * http://android.wooyd.org/JNIExample/#NWD1sCYeT-I
 * http://developer.android.com/training/articles/perf-jni.html and
 * http://www.ibm.com/developerworks/library/j-jni/
 * http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/invocation.html
 */

static JavaVM* jvm = nullptr;
// JNI Env bound on androids render thread (our native main thread)
static JNIEnv* jniRenderThreadEnv = nullptr;
static jobject tangramInstance = nullptr;
static jmethodID requestRenderMethodID = 0;
static jmethodID setRenderModeMethodID = 0;
static jmethodID startUrlRequestMID = 0;
static jmethodID cancelUrlRequestMID = 0;
static jmethodID getFontFilePath = 0;
static jmethodID getFontFallbackFilePath = 0;
static jmethodID onFeaturePickMID = 0;

static jclass hashmapClass = nullptr;
static jmethodID hashmapInitMID = 0;
static jmethodID hashmapPutMID = 0;

static AAssetManager* assetManager = nullptr;

static bool s_isContinuousRendering = false;

PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT = 0;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT = 0;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT = 0;

void bindJniEnvToThread(JNIEnv* jniEnv) {
    jniEnv->GetJavaVM(&jvm);
    jniRenderThreadEnv = jniEnv;
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

    if (hashmapClass) {
        jniEnv->DeleteGlobalRef(hashmapClass);
    }
    hashmapClass = (jclass)jniEnv->NewGlobalRef(jniEnv->FindClass("java/util/HashMap"));
    hashmapInitMID = jniEnv->GetMethodID(hashmapClass, "<init>", "()V");
    hashmapPutMID = jniEnv->GetMethodID(hashmapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

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

std::string systemFontFallbackPath(int _importance, int _weightHint) {

    JniThreadBinding jniEnv(jvm);

    jstring returnStr = (jstring) jniEnv->CallObjectMethod(tangramInstance, getFontFallbackFilePath, _importance, _weightHint);

    auto resultStr = stringFromJString(jniEnv, returnStr);
    jniEnv->DeleteLocalRef(returnStr);

    return resultStr;
}

std::string systemFontPath(const std::string& _family, const std::string& _weight, const std::string& _style) {

    JniThreadBinding jniEnv(jvm);

    std::string key = _family + "_" + _weight + "_" + _style;

    jstring jkey = jniEnv->NewStringUTF(key.c_str());
    jstring returnStr = (jstring) jniEnv->CallObjectMethod(tangramInstance, getFontFilePath, jkey);

    auto resultStr = stringFromJString(jniEnv, returnStr);
    jniEnv->DeleteLocalRef(returnStr);
    jniEnv->DeleteLocalRef(jkey);

    return resultStr;
}

void setContinuousRendering(bool _isContinuous) {

    s_isContinuousRendering = _isContinuous;

    JniThreadBinding jniEnv(jvm);

    jniEnv->CallVoidMethod(tangramInstance, setRenderModeMethodID, _isContinuous ? 1 : 0);
}

bool isContinuousRendering() {

    return s_isContinuousRendering;

}

unsigned char* bytesFromAssetManager(const char* _path, size_t& _size) {

    unsigned char* data = nullptr;

    AAsset* asset = AAssetManager_open(assetManager, _path, AASSET_MODE_UNKNOWN);

    if (asset == nullptr) {
        logMsg("Failed to open asset at path: %s\n", _path);
        _size = 0;
        return data;
    }

    _size = AAsset_getLength(asset);

    data = (unsigned char*) malloc(sizeof(unsigned char) * _size);

    int read = AAsset_read(asset, data, _size);

    if (read <= 0) {
        logMsg("Failed to read asset at path: %s\n", _path);
    }

    AAsset_close(asset);

    return data;
}

unsigned char* bytesFromFileSystem(const char* _path, size_t& _size) {

    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        _size = 0;
        return nullptr;
    }

    _size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*) malloc(sizeof(char) * _size);

    resource.read(cdata, _size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);

}

std::string stringFromFile(const char* _path) {

    size_t length = 0;
    unsigned char* bytes = bytesFromFile(_path, length);
    std::string out(reinterpret_cast<char*>(bytes), length);
    free(bytes);

    return out;

}

unsigned char* bytesFromFile(const char* _path, size_t& _size) {

    size_t len = strlen(_path);

    if (len > 0) {
        if (_path[0] == '/') {

            return bytesFromFileSystem(_path, _size);

        } else {
            // For unclear reasons, the AAssetManager will fail to open a file at
            // path "filepath" if the path is instead given as "./filepath"
            if (len >= 2 && (_path[0] == '.') && (_path[1] == '/')) { _path += 2; }

            return bytesFromAssetManager(_path, _size);
        }
    }
    _size = 0;
    return nullptr;
}

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {

    jstring jUrl = jniRenderThreadEnv->NewStringUTF(_url.c_str());

    // This is probably super dangerous. In order to pass a reference to our callback we have to convert it
    // to a Java type. We allocate a new callback object and then reinterpret the pointer to it as a Java long.
    // In Java, we associate this long with the current network request and pass it back to native code when
    // the request completes (either in onUrlSuccess or onUrlFailure), reinterpret the long back into a
    // pointer, call the callback function if the request succeeded, and delete the heap-allocated UrlCallback
    // to make sure nothing is leaked.
    jlong jCallbackPtr = reinterpret_cast<jlong>(new UrlCallback(_callback));

    jboolean methodResult = jniRenderThreadEnv->CallBooleanMethod(tangramInstance, startUrlRequestMID, jUrl, jCallbackPtr);

    return methodResult;
}

void cancelUrlRequest(const std::string& _url) {
    jstring jUrl = jniRenderThreadEnv->NewStringUTF(_url.c_str());
    jniRenderThreadEnv->CallVoidMethod(tangramInstance, cancelUrlRequestMID, jUrl);
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

void featurePickCallback(jobject listener, const std::vector<Tangram::TouchItem>& items) {

    JniThreadBinding jniEnv(jvm);

    auto result = items[0];
    auto properties = result.properties;
    auto position = result.position;

    jobject hashmap = jniEnv->NewObject(hashmapClass, hashmapInitMID);

    for (const auto& item : properties->items()) {
        jstring jkey = jniEnv->NewStringUTF(item.key.c_str());
        jstring jvalue = jniEnv->NewStringUTF(properties->asString(item.value).c_str());
        jniEnv->CallObjectMethod(hashmap, hashmapPutMID, jkey, jvalue);
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
