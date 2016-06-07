#pragma once

#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <functional>
#include <cstdio>

/* Print a formatted message to the console
 *
 * Uses printf syntax to write a string to stderr (or logcat, on Android)
 */
void logMsg(const char* fmt, ...);

/* Request that a new frame be rendered by the windowing system
 */
void requestRender();

/* If called with 'true', the windowing system will re-draw frames continuously;
 * otherwise new frames will only be drawn when 'requestRender' is called.
 */
void setContinuousRendering(bool _isContinuous);

bool isContinuousRendering();

/* get system path of a font file */
std::string systemFontPath(const std::string& _name, const std::string& _weight, const std::string& _face);

enum class PathType : char {
    absolute, // resolved relative to the filesystem root
    internal, // resolved relative to the application storage directory
    resource, // resolved relative to the resource root
};

/* Set a path to act as the resource root. All other resource paths will be resolved relative to this root.
 * The string returned is the path to the given file relative to the new root resource directory. */
std::string setResourceRoot(const char* _path);

/* Read a file as a string
 *
 * Opens the file at the _path, resolved with _type, and returns a string with its contents.
 * If the file cannot be found or read, the returned string is empty.
 */
std::string stringFromFile(const char* _path, PathType _type);

/* Read a file into memory
 *
 * Opens the file at _path, resolved with _type, then allocates and returns a pointer to memory
 * containing the contents of the file. The size of the memory in bytes is written to _size.
 * If the file cannot be read, nothing is allocated and nullptr is returned.
 */
unsigned char* bytesFromFile(const char* _path, PathType _type, unsigned int* _size);

/* Function type for receiving data from a successful network request */
using UrlCallback = std::function<void(std::vector<char>&&)>;

/* Start retrieving data from a URL asynchronously
 *
 * When the request is finished, the callback @_callback will be
 * run with the data that was retrieved from the URL @_url
 */
bool startUrlRequest(const std::string& _url, UrlCallback _callback);

/* Stop retrieving data from a URL that was previously requested
 */
void cancelUrlRequest(const std::string& _url);


/* Set the priority of the current thread. Priority is equivalent
 * to pthread niceness.
 */
void setCurrentThreadPriority(int priority);

/* Get the font fallback ordered by importance, 0 being the first fallback
 * (e.g. the fallback more willing resolve the glyph codepoint)
 */
std::string systemFontFallbackPath(int _importance, int _weightHint);

void initGLExtensions();

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

#define LOGD(fmt, ...)
#define LOGW(fmt, ...)
#define LOGE(fmt, ...)
#define LOGN(fmt, ...)
#define LOG(fmt, ...)
#define LOGS(fmt, ...)

