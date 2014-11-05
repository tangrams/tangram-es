#ifdef PLATFORM_ANDROID

#include "platform.h"

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <cstdarg>

AAssetManager* assetManager;

void setAssetManager(JNIEnv* _jniEnv, jobject _assetManager) {

    assetManager = AAssetManager_fromJava(_jniEnv, _assetManager);

}

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
    va_end(args);

}

int readInternalFile(const char* path, void*& buff, long& length) {

    // Open asset
    AAsset* asset = AAssetManager_open(assetManager, path, AASSET_MODE_STREAMING);
    
    // Get length of file
    length = AAsset_getLength(asset);

    // Read file
    int bytesRead = AAsset_read(asset, buff, length);

    // Clean up
    AAsset_close(asset);

    if (bytesRead >= 0) {
        return 0;
    } else {
        logMsg("Failed to open file at path: %s\n", path);
        return -1;
    }

}

#endif
