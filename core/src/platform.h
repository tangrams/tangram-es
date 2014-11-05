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

void logMsg(const char* fmt, ...);

std::string stringFromResource(const char* _path);
