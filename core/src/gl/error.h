#pragma once

#include "platform.h"
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

private:

    static std::unordered_map<GLenum, std::string> s_GlErrorCodesToStrings;

};

}
