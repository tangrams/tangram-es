#pragma once

#ifdef PLATFORM_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#ifdef PLATFORM_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#endif

#ifdef PLATFORM_OSX
#include <GLFW/glfw3.h>
/*
 * typedef to resolve name conflict in osx
 */
#define glClearDepthf glClearDepth
#define glDepthRangef glDepthRange
#endif
