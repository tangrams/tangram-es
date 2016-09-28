#pragma once
#include "epoxy/gl.h"

#if defined(PLATFORM_IOS) || defined(PLATFORM_WINDOWS)
#undef glDeleteVertexArrays
#undef glGenVertexArrays
#undef glBindVertexArray
#define glDeleteVertexArrays glDeleteVertexArraysOES
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#endif

#ifdef PLATFORM_LINUX
#undef glClearDepthf
#undef glDepthRangef
#define glClearDepthf glClearDepth
#define glDepthRangef glDepthRange
#endif

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS) || defined(PLATFORM_RPI) || defined(PLATFORM_WINDOWS)
#undef glMapBuffer
#undef glUnmapBuffer
#define glMapBuffer glMapBufferOES
#define glUnmapBuffer glUnmapBufferOES
#endif
