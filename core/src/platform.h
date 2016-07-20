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

/* Function type for a mapReady callback*/
using MapReady = std::function<void(void*)>;

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

/* Read a file as a string
 *
 * Opens the file at the _path and returns a string with its contents.
 * If the file cannot be found or read, the returned string is empty.
 */
std::string stringFromFile(const char* _path);

/* Read a file into memory
 *
 * Opens the file at _path then allocates and returns a pointer to memory
 * containing the contents of the file. The size of the memory in bytes is written to _size.
 * If the file cannot be read, nothing is allocated and nullptr is returned.
 */
unsigned char* bytesFromFile(const char* _path, size_t& _size);

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

void initPlatformFontSetup();

void initGLExtensions();
