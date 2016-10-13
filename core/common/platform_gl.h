#pragma once

#ifdef PLATFORM_ANDROID
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
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

// Dummy VertexArray functions
static void glBindVertexArray(GLuint array) {}
static void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {}
static void glGenVertexArrays(GLsizei n, GLuint *arrays) {}

#endif

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS) || defined(PLATFORM_RPI)
    #define glMapBuffer glMapBufferOES
    #define glUnmapBuffer glUnmapBufferOES
#endif
