#pragma once

#ifdef PLATFORM_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>
#include <cstdarg>
#endif

#ifdef PLATFORM_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include <cstdio>
#include <cstdarg>
#endif

#ifdef PLATFORM_OSX
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdarg>
#endif

void logMsg(const char* fmt, ...);

