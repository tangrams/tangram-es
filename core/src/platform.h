#pragma once

#ifdef PLATFORM_ANDROID
struct _JNIEnv;
typedef _JNIEnv JNIEnv;
class _jobject;
typedef _jobject* jobject;
void setAssetManager(JNIEnv* _jniEnv, jobject _assetManager);
#endif

#if (defined PLATFORM_IOS) && (defined __OBJC__)
#import "ViewController.h"
void setViewController(ViewController* _controller);
#endif

#include <string>

/* Print a formatted message to the console
 *
 * Uses printf syntax to write a string to stderr (or logcat, on Android)
 */
void logMsg(const char* fmt, ...);

/* Request that a new frame be rendered by the windowing system
 */
void requestRender();

/* Read a bundled resource file as a string
 * 
 * Opens the file at the given relative path and returns a string with its contents.
 * _path is the location of the file within the core/resources folder. If the file
 * cannot be found or read, the returned string is empty. 
 */
std::string stringFromResource(const char* _path);

/* Read and allocates size bytes of memory  
 *
 * Similarly to stringFromResource, _path is the location of file within 
 * core/resources. If the file cannot be read nothing is allocated and 
 * a nullptr is returned.
 * _size is is an in/out parameter to retrieve the size in bytes of the 
 * allocated file
 */ 
unsigned char* bytesFromResource(const char* _path, unsigned int* _size);
