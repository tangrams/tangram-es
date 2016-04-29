#include "error.h"

namespace Tangram {

std::unordered_map<GLenum, std::string> Error::s_GlErrorCodesToStrings = {
        {GL_NO_ERROR, "GL_NO_ERROR"},
        {GL_INVALID_ENUM, "GL_INVALID_ENUM"},
        {GL_INVALID_VALUE, "GL_INVALID_VALUE"},
        {GL_INVALID_OPERATION, "GL_INVALID_OPERATION"},
        {GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY"}
    };

bool Error::hadGlError(const std::string& _locationTag) {

    GLenum error = glGetError();

    if (error != GL_NO_ERROR) {

        std::string errorString = s_GlErrorCodesToStrings[error];

        LOGE("%s at %s", errorString.c_str(), _locationTag.c_str());

        return true;
    }

    return false;
}

void Error::glError(const char* stmt, const char* fname, int line) {
    GLenum err = glGetError();

    auto it = s_GlErrorCodesToStrings.find(err);

    if (it != s_GlErrorCodesToStrings.end() && err != GL_NO_ERROR) {
        LOGE("OpenGL error %s, at %s:%i - for %s\n", it->second.c_str(), fname, line, stmt);
    }
}

}
