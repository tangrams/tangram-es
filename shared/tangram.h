#pragma once

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "platform.h"

void initializeOpenGL();
void resizeViewport(int newWidth, int newHeight);
void renderFrame();

GLuint makeCompiledShader(const GLchar* src, GLenum type);
GLuint makeLinkedShaderProgram(const GLchar* vertexSrc, const GLchar* fragmentSrc);
