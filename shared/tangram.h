#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#ifdef PLATFORM_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>
#include <malloc.h>
#endif

#ifdef PLATFORM_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#endif

#ifdef PLATFORM_OSX
#include <GLFW/glfw3.h>
#endif

void logMsg(const char* fmt, const char* msg);
void initializeOpenGL();
void resizeViewport(int newWidth, int newHeight);
void renderFrame();

GLuint makeCompiledShader(const GLchar* src, GLenum type);
GLuint makeLinkedShaderProgram(const GLchar* vertexSrc, const GLchar* fragmentSrc);
