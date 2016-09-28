#pragma once

#ifdef TANGRAM_USE_EPOXY

#include "epoxy/gl.h"

#elif defined(PLATFORM_ANDROID) // TANGRAM_CORE_EXPORT_NEEDED

#include <GLES2/gl2platform.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
// defined in platform_android.cpp
extern PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT;
extern PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT;
extern PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT;

#elif defined(PLATFORM_IOS) // PLATFORM_ANDROID

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#elif defined(PLATFORM_WINDOWS) // PLATFORM_IOS

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#elif defined(PLATFORM_OSX) // PLATFORM_WINDOWS

#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

#elif defined(PLATFORM_RPI) // PLATFORM_OSX

//  Broadcom hardware library for hijacking the GPU card without window manager
//
#include "bcm_host.h"

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#elif defined(PLATFORM_LINUX) // PLATFORM_RPI

#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#endif // PLATFORM_LINUX

// Ending of includes

#ifdef PLATFORM_ANDROID
#define glDeleteVertexArrays glDeleteVertexArraysOESEXT
#define glGenVertexArrays glGenVertexArraysOESEXT
#define glBindVertexArray glBindVertexArrayOESEXT
#endif

#if defined(PLATFORM_IOS) || defined(PLATFORM_WINDOWS)
#ifdef TANGRAM_USE_EPOXY
#undef glDeleteVertexArrays
#undef glGenVertexArrays
#undef glBindVertexArray
#endif
#define glDeleteVertexArrays glDeleteVertexArraysOES
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#endif

#ifdef PLATFORM_LINUX
#ifdef TANGRAM_USE_EPOXY
    #undef glClearDepthf
    #undef glDepthRangef
#endif
#define glClearDepthf glClearDepth
#define glDepthRangef glDepthRange
#endif

#ifdef PLATFORM_OSX
/*
 * typedef to resolve name conflict in osx
 */
#define glClearDepthf glClearDepth
#define glDepthRangef glDepthRange
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#endif

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS) || defined(PLATFORM_RPI) || defined(PLATFORM_WINDOWS)
#ifdef TANGRAM_USE_EPOXY
    #undef glMapBuffer
    #undef glUnmapBuffer
#endif
    #define glMapBuffer glMapBufferOES
    #define glUnmapBuffer glUnmapBufferOES
#endif
