#include "platform_magnum.h"
#include "gl/hardware.h"
#include "log.h"
#include <algorithm>
#include <stdarg.h>
#include <stdio.h>

namespace Tangram {
void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}


} // namespace Tangram
