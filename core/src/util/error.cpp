#include "error.h"

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

        logMsg("OpenGL Error: %s at %s", errorString.c_str(), _locationTag.c_str());

        return true;
    }

    return false;
}
