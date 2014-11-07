#ifdef PLATFORM_ANDROID

#include "platform.h"

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <cstdarg>

static AAssetManager* assetManager;

void setAssetManager(JNIEnv* _jniEnv, jobject _assetManager) {

    assetManager = AAssetManager_fromJava(_jniEnv, _assetManager);

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

#endif
