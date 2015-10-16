#pragma once

#if defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
#define DESKTOP_GL true
#else
#define DESKTOP_GL false
#endif

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
#define glMapBuffer glMapBufferOES
#define glUnmapBuffer glUnmapBufferOES
#endif

#ifdef PLATFORM_ANDROID
#include <GLES2/gl2platform.h>
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h> // dlopen, dlsym

extern PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT;
extern PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT;
extern PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT;

#define glDeleteVertexArrays glDeleteVertexArraysOESEXT
#define glGenVertexArrays glGenVertexArraysOESEXT
#define glBindVertexArray glBindVertexArrayOESEXT
#endif

#ifdef PLATFORM_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#define glDeleteVertexArrays glDeleteVertexArraysOES
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#endif

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS)
#define GL_WRITE_ONLY GL_WRITE_ONLY_OES
#endif

#ifdef PLATFORM_OSX
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
/*
 * typedef to resolve name conflict in osx
 */
#define glClearDepthf glClearDepth
#define glDepthRangef glDepthRange
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#endif

#ifdef PLATFORM_LINUX
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#endif

#ifdef PLATFORM_RPI
//  Broadcom hardware library for hijacking the GPU card without window manager
//
#include "bcm_host.h"

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#include "extension.h"

