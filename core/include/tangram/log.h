#pragma once

#include "platform.h"

#include <atomic>
#include <cstring>

/*
 * Log utilities:
 * LOGD: Debug log, LOG_LEVEL >= 3
 * LOGW: Warning log, LOG_LEVEL >= 2
 * LOGE: Error log, LOG_LEVEL >= 1
 * LOGN: Notification log (displayed once), LOG_LEVEL >= 0
 * LOG: Default log, LOG_LEVEL >= 0
 * LOGS: Screen log, no LOG_LEVEL
 */

//#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
// From: https://blog.galowicz.de/2016/02/20/short_file_macro/
static constexpr const char * past_last_slash(const char * const str, const char * const last_slash) {
    return *str == '\0' ? last_slash :
        *str == '/'  ? past_last_slash(str + 1, str + 1) :
        past_last_slash(str + 1, last_slash);
}

static constexpr const char * past_last_slash(const char * const str) {
    return past_last_slash(str, str);
}

#define __FILENAME__ ({constexpr const char * const sf__ {past_last_slash(__FILE__)}; sf__;})

#define TANGRAM_MAX_BUFFER_LOG_SIZE 99999

#if LOG_LEVEL >= 3
#define LOGD(fmt, ...) \
do { Tangram::logMsg("DEBUG %s:%d: " fmt "\n", __FILENAME__, __LINE__, ## __VA_ARGS__); } while(0)
#else
#define LOGD(fmt, ...)
#endif

#if LOG_LEVEL >= 2
#define LOGW(fmt, ...) \
do { Tangram::logMsg("WARNING %s:%d: " fmt "\n", __FILENAME__, __LINE__, ## __VA_ARGS__); } while(0)
#else
#define LOGW(fmt, ...)
#endif

#if LOG_LEVEL >= 1
#define LOGE(fmt, ...) \
do { Tangram::logMsg("ERROR %s:%d: " fmt "\n", __FILENAME__, __LINE__, ## __VA_ARGS__); } while(0)
#else
#define LOGE(fmt, ...)
#endif

#if LOG_LEVEL >= 0
// The 'please notice but don't be too annoying' logger:
#define LOGN(fmt, ...)                                                                                                 \
    do {                                                                                                               \
        static std::atomic<size_t> _lock(0);                                                            \
        if (_lock++ < 42) { logMsg("NOTIFY %s:%d: " fmt "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); } \
    } while (0)

#define LOG(fmt, ...)                                                   \
    do { Tangram::logMsg("TANGRAM %s:%d: " fmt "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); } while (0)
#else
#define LOG(fmt, ...)
#define LOGN(fmt, ...)
#endif
