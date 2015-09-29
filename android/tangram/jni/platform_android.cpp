#ifdef PLATFORM_ANDROID

#include "platform.h"

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include <cstdarg>

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <fstream>

/* Followed the following document for JavaVM tips when used with native threads
 * http://android.wooyd.org/JNIExample/#NWD1sCYeT-I
 * http://developer.android.com/training/articles/perf-jni.html and
 * http://www.ibm.com/developerworks/library/j-jni/
 * http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/invocation.html
 */

static JavaVM* jvm;
// JNI Env bound on androids render thread (our native main thread)
static JNIEnv* jniRenderThreadEnv;
static jobject tangramInstance;
static jmethodID requestRenderMethodID;
static jmethodID setRenderModeMethodID;
static jmethodID startUrlRequestMID;
static jmethodID cancelUrlRequestMID;
static jmethodID getFontFilePath;
static AAssetManager* assetManager;

static bool s_isContinuousRendering = false;
static bool s_useInternalResources = true;
static std::string s_resourceRoot;

void setupJniEnv(JNIEnv* _jniEnv, jobject _tangramInstance, jobject _assetManager) {
    _jniEnv->GetJavaVM(&jvm);
    JNIEnv* jniEnv = _jniEnv;
    jniRenderThreadEnv = _jniEnv;

    tangramInstance = jniEnv->NewGlobalRef(_tangramInstance);
    jclass tangramClass = jniEnv->FindClass("com/mapzen/tangram/MapController");
    startUrlRequestMID = jniEnv->GetMethodID(tangramClass, "startUrlRequest", "(Ljava/lang/String;J)Z");
    cancelUrlRequestMID = jniEnv->GetMethodID(tangramClass, "cancelUrlRequest", "(Ljava/lang/String;)V");
    getFontFilePath = jniEnv->GetMethodID(tangramClass, "getFontFilePath", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
        requestRenderMethodID = _jniEnv->GetMethodID(tangramClass, "requestRender", "()V");
    setRenderModeMethodID = _jniEnv->GetMethodID(tangramClass, "setRenderMode", "(I)V");

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
};

void requestRender() {

    JniThreadBinding jniEnv(jvm);

    jniEnv->CallVoidMethod(tangramInstance, requestRenderMethodID);
}

std::string systemFontPath(const std::string& _family, const std::string& _weight, const std::string& _style) {

    JniThreadBinding jniEnv(jvm);

    jstring jfamily = jniEnv->NewStringUTF(_family.c_str());
    jstring jweight = jniEnv->NewStringUTF(_weight.c_str());
    jstring jstyle = jniEnv->NewStringUTF(_style.c_str());
    jstring returnStr = (jstring) jniEnv->CallObjectMethod(tangramInstance, getFontFilePath, jfamily, jweight, jstyle);

    size_t length = jniEnv->GetStringUTFLength(returnStr);
    std::string fontPath = std::string(length, 0);
    jniEnv->GetStringUTFRegion(returnStr, 0, length, &fontPath[0]);
    return fontPath;
}

void setContinuousRendering(bool _isContinuous) {

    s_isContinuousRendering = _isContinuous;

    JniThreadBinding jniEnv(jvm);

    jniEnv->CallVoidMethod(tangramInstance, requestRenderMethodID, _isContinuous ? 1 : 0);
}

bool isContinuousRendering() {

    return s_isContinuousRendering;

}

std::string setResourceRoot(const char* _path) {

    s_resourceRoot = std::string(dirname(_path));

    s_useInternalResources = (*_path != '/');

    // For unclear reasons, the AAssetManager will fail to open a file at
    // path "filepath" if the path is instead given as "./filepath", so in
    // cases where dirname returns "." we simply use an empty string. For
    // all other cases, we add a "/" for appending relative paths.
    if (!s_resourceRoot.empty() && s_resourceRoot.front() == '.') {
        s_resourceRoot = "";
    } else {
        s_resourceRoot += '/';
    }

    return std::string(basename(_path));

}

unsigned char* bytesFromAssetManager(const char* _path, unsigned int* _size) {

    unsigned char* data = nullptr;

    AAsset* asset = AAssetManager_open(assetManager, _path, AASSET_MODE_UNKNOWN);

    if (asset == nullptr) {
        logMsg("Failed to open asset at path: %s\n", _path);
        *_size = 0;
        return data;
    }

    *_size = AAsset_getLength(asset);

    data = (unsigned char*) malloc(sizeof(unsigned char) * (*_size));

    int read = AAsset_read(asset, data, *_size);

    if (read <= 0) {
        logMsg("Failed to read asset at path: %s\n", _path);
    }

    AAsset_close(asset);

    return data;
}

std::string stringFromResource(const char* _path) {

    std::string path = s_resourceRoot + _path;
    unsigned int length = 0;
    unsigned char* bytes = nullptr;

    if (s_useInternalResources) {
        bytes = bytesFromResource(path.c_str(), &length);
    } else {
        bytes = bytesFromFileSystem(path.c_str(), &length);
    }

    std::string out(reinterpret_cast<char*>(bytes), length);
    free(bytes);

    return out;

}

unsigned char* bytesFromFileSystem(const char* _path, unsigned int* _size) {

    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        *_size = 0;
        return nullptr;
    }

    *_size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*) malloc(sizeof(char) * (*_size));

    resource.read(cdata, *_size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);

}

unsigned char* bytesFromResource(const char* _path, unsigned int* _size) {

    std::string path = s_resourceRoot + _path;
    if (s_useInternalResources) {
        return bytesFromAssetManager(path.c_str(), _size);
    } else {
        return bytesFromFileSystem(path.c_str(), _size);
    }

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

    UrlCallback* callback = reinterpret_cast<UrlCallback*>(_jCallbackPtr);
    delete callback;

}

void setCurrentThreadPriority(int priority) {
    int  tid = gettid();
    setpriority(PRIO_PROCESS, tid, priority);
}


#endif
