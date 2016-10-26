#pragma once

#include "gl.h"
#include <string>
#include <unordered_map>

namespace Tangram {

class Error {

public:

    /*
     * hadGlError - checks OpenGL for any recorded errors. If no errors are found, it returns false.
     * If an error is found, it prints the GL error enum combined with the location tag passed in,
     * then returns true. This is intended to be used infrequently, in places where errors are likely or known.
     */
    static bool hadGlError(const std::string& _locationTag);

    static void glError(const char* stmt, const char* fname, int line);

private:

    static std::unordered_map<GLenum, std::string> s_GlErrorCodesToStrings;

};

#ifdef DEBUG
#define GL_CHECK(STMT) do { STMT; Tangram::Error::glError(#STMT, __FILE__, __LINE__); } while (0)
#else
#define GL_CHECK(STMT) do { STMT; } while (0)
#endif

}
