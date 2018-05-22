#pragma once

#ifdef TANGRAM_ANDROID
#include <GLES2/gl2platform.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
// Values defined in androidPlatform.cpp
extern PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT;
extern PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT;
extern PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT;

#define glDeleteVertexArrays glDeleteVertexArraysOESEXT
#define glGenVertexArrays glGenVertexArraysOESEXT
#define glBindVertexArray glBindVertexArrayOESEXT
#endif // TANGRAM_ANDROID

#ifdef TANGRAM_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#define glDeleteVertexArrays glDeleteVertexArraysOES
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#endif // TANGRAM_IOS

#ifdef TANGRAM_OSX
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
// Resolve aliased names in OS X
#define glClearDepthf glClearDepth
#define glDepthRangef glDepthRange
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#endif // TANGRAM_OSX

#if defined(TANGRAM_LINUX) || defined(TANGRAM_WINDOWS)
#define GL_GLEXT_PROTOTYPES
#ifdef TANGRAM_WINDOWS
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
// Add missing stuff
#define glDepthRangef(a, b) glDepthRange((double)(a), (double)(b))
#define glClearDepthf(a) glClearDepth((double)(a))
#endif // TANGRAM_WINDOWS
#include <GLFW/glfw3.h>
#endif // defined(TANGRAM_LINUX) || defined(TANGRAM_WINDOWS)

#ifdef TANGRAM_RPI
// Broadcom library for direct GPU access
#include "bcm_host.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

// Dummy VertexArray functions
static void glBindVertexArray(GLuint array) {}
static void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {}
static void glGenVertexArrays(GLsizei n, GLuint *arrays) {}

#endif // TANGRAM_RPI

#if defined(TANGRAM_ANDROID) || defined(TANGRAM_IOS) || defined(TANGRAM_RPI)
    #define glMapBuffer glMapBufferOES
    #define glUnmapBuffer glUnmapBufferOES
#endif // defined(TANGRAM_ANDROID) || defined(TANGRAM_IOS) || defined(TANGRAM_RPI)
