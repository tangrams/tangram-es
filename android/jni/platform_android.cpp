#ifdef PLATFORM_ANDROID

#include "platform.h"

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <jni.h>
#include <cstdarg>

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
static jmethodID networkRequestMID;
static jmethodID cancelNetworkRequestMID;
static AAssetManager* assetManager;

static bool s_isContinuousRendering = false;

static std::function<void(std::vector<char>&&, TileID, int)> networkCallback;

void setupJniEnv(JNIEnv* _jniEnv, jobject _tangramInstance, jobject _assetManager) {
	_jniEnv->GetJavaVM(&jvm);
    jniEnv = _jniEnv;

    tangramInstance = jniEnv->NewGlobalRef(_tangramInstance);
    jclass tangramClass = jniEnv->FindClass("com/mapzen/tangram/Tangram");
    networkRequestMID = jniEnv->GetMethodID(tangramClass, "networkRequest", "(Ljava/lang/String;IIII)Z");
    cancelNetworkRequestMID = jniEnv->GetMethodID(tangramClass, "cancelNetworkRequest", "(Ljava/lang/String;)V");
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

bool startNetworkRequest(const std::string& _url, const TileID& _tileID, const int _dataSourceID) {
    
    jstring jUrl = jniEnv->NewStringUTF(_url.c_str());

    jboolean methodResult = jniEnv->CallBooleanMethod(tangramInstance, networkRequestMID, jUrl, (jint)_tileID.x, (jint)_tileID.y, (jint)_tileID.z, (jint)_dataSourceID);

    if(!methodResult) {
        logMsg("\"networkRequest\" returned false");
        return methodResult;
    }

    return methodResult;
}

void cancelNetworkRequest(const std::string& _url) {

    jstring jUrl = jniEnv->NewStringUTF(_url.c_str());
    jniEnv->CallVoidMethod(tangramInstance, cancelNetworkRequestMID, jUrl);

}

void setNetworkRequestCallback(std::function<void(std::vector<char>&&, TileID, int)>&& _callback) {

    networkCallback = _callback;

}

void networkDataBridge(JNIEnv* _jniEnv, jbyteArray _jFetchedBytes, int _tileIDx, int _tileIDy, int _tileIDz, int _dataSourceID) {

    int dataLength = _jniEnv->GetArrayLength(_jFetchedBytes);

    std::vector<char> rawData;
    rawData.resize(dataLength);

    _jniEnv->GetByteArrayRegion(_jFetchedBytes, 0, dataLength, reinterpret_cast<jbyte*>(rawData.data()));

    networkCallback(std::move(rawData), TileID(_tileIDx, _tileIDy, _tileIDz), _dataSourceID);

}


#endif
