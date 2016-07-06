#pragma once

#include "platform.h"

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
#define LOGN(fmt, ...) do {                                                         \
    static std::vector<std::string> logs;                                           \
    static char buffer[TANGRAM_MAX_BUFFER_LOG_SIZE];                                \
    snprintf(buffer, TANGRAM_MAX_BUFFER_LOG_SIZE - 1, "%s:%d:" fmt, __FILENAME__,   \
        __LINE__, ## __VA_ARGS__);                                                  \
    std::string log(buffer);                                                        \
    if (std::find(logs.begin(), logs.end(), log) == logs.end()) {                   \
        logs.push_back(log);                                                        \
        logMsg("NOTIFY %s:%d: " fmt "\n", __FILENAME__, __LINE__, ## __VA_ARGS__);  \
    }                                                                               \
} while (0)
#define LOG(fmt, ...) \
do { logMsg("TANGRAM %s:%d: " fmt "\n", __FILENAME__, __LINE__, ## __VA_ARGS__); } while(0)
#else
#define LOGN(fmt, ...)
#define LOG(fmt, ...)
#endif

#define LOGS(fmt, ...) \
do { TextDisplay::Instance().log(fmt, ## __VA_ARGS__); } while(0)
