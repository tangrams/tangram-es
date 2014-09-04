#ifdef PLATFORM_IOS

#include "platform.h"

void logMsg(const char* fmt, const char* msg) {
    fprintf(stderr, fmt, msg);
}

#endif