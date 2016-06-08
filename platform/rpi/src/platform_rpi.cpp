#include "gl.h"
#include "context.h"
#include "urlWorker.h"

#include <libgen.h>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include "platform_rpi.h"

static bool s_isContinuousRendering = false;
static std::string s_resourceRoot;

#include "platform_common.h"

void requestRender() {
    setRenderRequest(true);
}

void setContinuousRendering(bool _isContinuous) {
    s_isContinuousRendering = _isContinuous;
}

bool isContinuousRendering() {
    return s_isContinuousRendering;
}
std::string resolvePath(const char* _path, PathType _type) {

    switch (_type) {
    case PathType::absolute:
    case PathType::internal:
        return std::string(_path);
    case PathType::resource:
        return s_resourceRoot + _path;
    }
}

void setCurrentThreadPriority(int priority) {}

void initGLExtensions() {}

// Dummy VertexArray functions
GL_APICALL void GL_APIENTRY glBindVertexArray(GLuint array) {}
GL_APICALL void GL_APIENTRY glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {}
GL_APICALL void GL_APIENTRY glGenVertexArrays(GLsizei n, GLuint *arrays) {}

