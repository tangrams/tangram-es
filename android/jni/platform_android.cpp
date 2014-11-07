#include "platform.h"

#include <android/log.h>
#include <cstdarg>

void logMsg(const char* fmt, ...) {

    va_list args;
    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
    va_end(args);

}
