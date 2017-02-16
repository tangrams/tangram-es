#pragma once

#include "debug/textDisplay.h"
#include "platform.h"

#include <atomic>

/*
 * Log utilities:
 * LOGD: Debug log, LOG_LEVEL >= 3
 * LOGW: Warning log, LOG_LEVEL >= 2
 * LOGE: Error log, LOG_LEVEL >= 1
 * LOGN: Notification log (displayed once), LOG_LEVEL >= 0
 * LOG: Default log, LOG_LEVEL >= 0
 * LOGS: Screen log, no LOG_LEVEL
 */

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define TANGRAM_MAX_BUFFER_LOG_SIZE 99999

#if LOG_LEVEL >= 3
#define LOGD(fmt, ...) \
do { logMsg("DEBUG %s:%d: " fmt "\n", __FILENAME__, __LINE__, ## __VA_ARGS__); } while(0)
#else
#define LOGD(fmt, ...)
#endif

#if LOG_LEVEL >= 2
#define LOGW(fmt, ...) \
do { logMsg("WARNING %s:%d: " fmt "\n", __FILENAME__, __LINE__, ## __VA_ARGS__); } while(0)
#else
#define LOGW(fmt, ...)
#endif

#if LOG_LEVEL >= 1
#define LOGE(fmt, ...) \
do { logMsg("ERROR %s:%d: " fmt "\n", __FILENAME__, __LINE__, ## __VA_ARGS__); } while(0)
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
    do { logMsg("TANGRAM %s:%d: " fmt "\n", __FILENAME__, __LINE__, ##__VA_ARGS__); } while (0)
#else
#define LOG(fmt, ...)
#define LOGN(fmt, ...)
#endif

#define LOGS(fmt, ...) \
do { TextDisplay::Instance().log(fmt, ## __VA_ARGS__); } while(0)
