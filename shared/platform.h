#pragma once

#ifdef PLATFORM_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>
#include <malloc.h>
#endif

#ifdef PLATFORM_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include <stdio.h>
#endif

#ifdef PLATFORM_OSX
#include <GLFW/glfw3.h>
#include <stdio.h>
#endif

void logMsg(const char* fmt, const char* msg);