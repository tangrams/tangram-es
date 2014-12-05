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

#ifdef PLATFORM_RPI
//  Broadcom hardware library for hijacking the GPU card with out window manager
//
#include "bcm_host.h"

//  OpenGL ES 2.0
//
#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#endif
