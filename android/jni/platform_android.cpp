#ifdef PLATFORM_ANDROID

#include "platform.h"

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <cstdarg>

static JavaVM* jvm;
static jobject tangramObj;
static jmethodID requestRenderMethodID;
static AAssetManager* assetManager;
static bool s_isContinuousRendering = false;

void jniInit(JNIEnv* _jniEnv, jobject _tangramInstance, jobject _assetManager) {

    assetManager = AAssetManager_fromJava(_jniEnv, _assetManager);

    if (assetManager == nullptr) {
        logMsg("ERROR: Could not obtain Asset Manager reference\n");
    }

    _jniEnv->GetJavaVM(&jvm);
    tangramObj = _jniEnv->NewGlobalRef(_tangramInstance);
    
}

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
    va_end(args);

}

void requestRender() {
    
    JNIEnv *jniEnv;

    bool isAttached = false;
    int status = jvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6);
    if(status == JNI_EDETACHED) {
        status = jvm->AttachCurrentThread(&jniEnv, NULL);
        isAttached = true;
    }
    jclass tangramClass = jniEnv->GetObjectClass(tangramObj);
    requestRenderMethodID = jniEnv->GetMethodID(tangramClass, "requestRender", "()V");
    jniEnv->CallVoidMethod(tangramObj, requestRenderMethodID);
    if(isAttached) {
        jvm->DetachCurrentThread();
    }
}

void setContinuousRendering(bool _isContinuous) {

    s_isContinuousRendering = _isContinuous;

    JNIEnv *jniEnv;

    bool isAttached = false;
    int status = jvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6);
    if(status == JNI_EDETACHED) {
        status = jvm->AttachCurrentThread(&jniEnv, NULL);
        isAttached = true;
    }
    jclass tangramClass = jniEnv->GetObjectClass(tangramObj);
    requestRenderMethodID = jniEnv->GetMethodID(tangramClass, "setRenderMode", "(I)V");
    jniEnv->CallVoidMethod(tangramObj, requestRenderMethodID, _isContinuous ? 1 : 0);
    if(isAttached) {
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

#endif
