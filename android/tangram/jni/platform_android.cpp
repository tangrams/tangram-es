#ifdef PLATFORM_ANDROID

#include "platform.h"

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include <cstdarg>

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
static JNIEnv* jniEnv;
static jobject tangramInstance;
static jmethodID requestRenderMethodID;
static jmethodID setRenderModeMethodID;
static jmethodID startUrlRequestMID;
static jmethodID cancelUrlRequestMID;
static AAssetManager* assetManager;

static bool s_isContinuousRendering = false;

void setupJniEnv(JNIEnv* _jniEnv, jobject _tangramInstance, jobject _assetManager) {
	_jniEnv->GetJavaVM(&jvm);
    jniEnv = _jniEnv;

    tangramInstance = jniEnv->NewGlobalRef(_tangramInstance);
    jclass tangramClass = jniEnv->FindClass("com/mapzen/tangram/MapController");
    startUrlRequestMID = jniEnv->GetMethodID(tangramClass, "startUrlRequest", "(Ljava/lang/String;J)Z");
    cancelUrlRequestMID = jniEnv->GetMethodID(tangramClass, "cancelUrlRequest", "(Ljava/lang/String;)V");
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

void requestRender() {

    JNIEnv *jniEnv;
    int status = jvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6);
    if(status == JNI_EDETACHED) {
        jvm->AttachCurrentThread(&jniEnv, NULL);
    }

    jniEnv->CallVoidMethod(tangramInstance, requestRenderMethodID);

    if(status == JNI_EDETACHED) {
        jvm->DetachCurrentThread();
    }
}

std::string deviceFontsPath() {
    return "/system/fonts/";
}

std::string constructFontFilename(const std::string& _name, const std::string& _weight, const std::string& _face) {
    std::string fontFilename;
    fontFilename = _name + "-" + _weight + _face;
    return fontFilename;
}

bool parseTypeFaceFontsInfo(const std::string& _typefaceInput, std::string& _fontName, float& _fontSize) {

    char str[4][40];
    float size;
    int num = sscanf(_typefaceInput.c_str(), "%s %s %s %s", str[0], str[1], str[2], str[3]);
    switch(num) {
        case 1:
            _fontName = str[0];
            break;
        case 2:
            try {
                size = std::stof(std::string(str[0]));
                _fontSize = size;
                _fontName = str[1];
            } catch (const std::invalid_argument& e) {
                _fontName = std::string(str[1]) + "-" + std::string(str[0]);
            }
            break;
        case 3:
            try {
                size = std::stof(std::string(str[1]));
                _fontSize = size;
                _fontName = std::string(str[2]) + "-" + std::string(str[0]);
            } catch (const std::invalid_argument& e) {
                _fontName = std::string(str[2]) + "-" + std::string(str[1]) + std::string(str[0]);
            }
            break;
        case 4:
            try {
                size = std::stof(std::string(str[2]));
                _fontSize = size;
                _fontName = std::string(str[3]) + "-" + std::string(str[1]) + std::string(str[0]);
            } catch (const std::invalid_argument& e) {
                _fontName = std::string(str[3]) + "-" + std::string(str[2]) + std::string(str[1]) + std::string(str[0]);
            }
            break;
        case 0:
        default:
            return false;
    }
    return true;
}

void setContinuousRendering(bool _isContinuous) {

    s_isContinuousRendering = _isContinuous;

    JNIEnv *jniEnv;
    int status = jvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6);
    if(status == JNI_EDETACHED) {
        jvm->AttachCurrentThread(&jniEnv, NULL);
    }

    jniEnv->CallVoidMethod(tangramInstance, requestRenderMethodID, _isContinuous ? 1 : 0);

    if(status == JNI_EDETACHED) {
        jvm->DetachCurrentThread();
    }

}

bool isContinuousRendering() {

    return s_isContinuousRendering;

}

std::string stringFromResource(const char* _path) {

    std::string out;

    // Open asset
    AAsset* asset = AAssetManager_open(assetManager, _path, AASSET_MODE_STREAMING);

    if (asset == nullptr) {
        logMsg("Failed to open asset at path: %s\n", _path);
        return out;
    }

    // Allocate string
    int length = AAsset_getLength(asset);
    out.resize(length);

    // Read data
    int read = AAsset_read(asset, &out.front(), length);

    // Clean up
    AAsset_close(asset);

    if (read <= 0) {
        logMsg("Failed to read asset at path: %s\n", _path);
    }

    return out;

}

unsigned char* bytesFromExtMemory(const char* _path, unsigned int* _size) {

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

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {

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

    UrlCallback* callback = reinterpret_cast<UrlCallback*>(_jCallbackPtr);
    delete callback;

}

void setCurrentThreadPriority(int priority) {
    int  tid = gettid();
    setpriority(PRIO_PROCESS, tid, priority);
}


#endif
