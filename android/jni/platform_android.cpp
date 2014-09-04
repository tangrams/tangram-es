#ifdef PLATFORM_ANDROID

#include "platform.h"

void logMsg(const char* fmt, const char* msg) {
    __android_log_print(ANDROID_LOG_DEBUG, "Tangram", fmt, msg);
}

#endif