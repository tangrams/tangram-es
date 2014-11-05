#pragma once

#ifdef PLATFORM_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
struct _JNIEnv;
typedef _JNIEnv JNIEnv;
class _jobject;
typedef _jobject* jobject;
void setAssetManager(JNIEnv* _jniEnv, jobject _assetManager);
#endif

#ifdef PLATFORM_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#endif

#ifdef PLATFORM_OSX
#include <GLFW/glfw3.h>
#endif

#include <string>

/* Print a formatted message to the console
 *
 * Uses printf syntax to write a string to stderr (or logcat, on Android)
 */
void logMsg(const char* fmt, ...);

/* Read a bundled resource file as a string
 * 
 * Opens the file at the given relative path and returns a string with its contents.
 * _path is the location of the file within the core/resources folder. If the file
 * cannot be found or read, the returned string is empty. 
 */
std::string stringFromResource(const char* _path);
