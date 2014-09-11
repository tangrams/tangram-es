#pragma once

#include "platform.h"
#include <string>
#include <vector>
#include <unordered_map>

class ShaderProgram {

public:

    ShaderProgram();
    virtual ~ShaderProgram();

    GLint getGlProgram() { return m_glProgram; };
    GLint getAttribLocation(const std::string& _attribName);

    bool buildFromSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc);

    void use();

private:

    static GLint s_activeGlProgram;

    GLint m_glProgram;
    GLint m_glFragmentShader;
    GLint m_glVertexShader;
    std::unordered_map<std::string, GLint> m_attribMap;
    std::string m_fragmentShaderSource;
    std::string m_vertexShaderSource;

    GLint makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader);
    GLint makeCompiledShader(const std::string& _src, GLenum _type);

};
