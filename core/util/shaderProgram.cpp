#include "shaderProgram.h"

GLint ShaderProgram::s_activeGlProgram = 0;

ShaderProgram::ShaderProgram() {

    m_glProgram = 0;
    m_glFragmentShader = 0;
    m_glVertexShader = 0;

}

ShaderProgram::~ShaderProgram() {

    if (m_glProgram != 0) {
        glDeleteProgram(m_glProgram);
    }

    if (m_glFragmentShader != 0) {
        glDeleteShader(m_glFragmentShader);
    }

    if (m_glVertexShader != 0) {
        glDeleteShader(m_glVertexShader);
    }

    m_attribMap.clear();

}

GLint ShaderProgram::getAttribLocation(const std::string& _attribName) {

    // Get uniform location at this key, or create one valued at zero if absent
    GLint location = m_attribMap[_attribName];

    // Zero means this is a new entry
    if (location == 0) {

        // Get the actual location from OpenGL
        location = glGetAttribLocation(m_glProgram, _attribName.c_str());

        // -1 Means there was an error in OpenGL retrieving the location
        if (location == -1) {
            // Print information about the error and leave the value at zero
            Error::hadGlError("ShaderProgram::getAttribLocation");
        } else {
            // Found the location, so save it as the value for this key
            m_attribMap[_attribName] = location;
        }
    }

    return location;

}

GLint ShaderProgram::getUniformLocation(const std::string& _uniformName) {

    // Get uniform location at this key, or create one valued at zero if absent
    GLint location = m_uniformMap[_uniformName];

    // Zero means this is a new entry
    if (location == 0) {

        // Get the actual location from OpenGL
        location = glGetUniformLocation(m_glProgram, _uniformName.c_str());

        // -1 Means there was an error in OpenGL retrieving the location
        if (location == -1) {
            // Print information about the error and leave the value at zero
            Error::hadGlError("ShaderProgram::getUniformLocation");
        } else {
            // Found the location, so save it as the value for this key
            m_uniformMap[_uniformName] = location;
        }
    }

    return location;

}

void ShaderProgram::use() {

    if (m_glProgram != 0 && m_glProgram != s_activeGlProgram) {
        glUseProgram(m_glProgram);
        s_activeGlProgram = m_glProgram;
    }
}

bool ShaderProgram::buildFromSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc) {

    // Try to compile vertex and fragment shaders, releasing resources and quiting on failure

    GLint vertexShader = makeCompiledShader(_vertSrc, GL_VERTEX_SHADER);

    if (vertexShader == 0) {
        return false;
    }

    GLint fragmentShader = makeCompiledShader(_fragSrc, GL_FRAGMENT_SHADER);

    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    // Try to link shaders into a program, releasing resources and quiting on failure

    GLint program = makeLinkedShaderProgram(fragmentShader, vertexShader);

    if (program == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // New shaders linked successfully, so replace old shaders and program

    glDeleteShader(m_glFragmentShader);
    glDeleteShader(m_glVertexShader);
    glDeleteProgram(m_glProgram);

    m_glFragmentShader = fragmentShader;
    m_glVertexShader = vertexShader;
    m_glProgram = program;

    // Make copies of the shader source code inputs, for this program to keep

    m_fragmentShaderSource = std::string(_fragSrc);
    m_vertexShaderSource = std::string(_vertSrc);

    return true;

}

GLint ShaderProgram::makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader) {

    GLint program = glCreateProgram();
    glAttachShader(program, _fragShader);
    glAttachShader(program, _vertShader);
    glLinkProgram(program);

    GLint isLinked;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);

    if (isLinked == GL_FALSE) {
        GLint infoLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            glGetProgramInfoLog(program, infoLength, NULL, &infoLog[0]);
            logMsg("Error linking program:\n%s\n", &infoLog[0]);
        }
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

GLint ShaderProgram::makeCompiledShader(const std::string& _src, GLenum _type) {

    GLuint shader = glCreateShader(_type);
    const GLchar* source = (const GLchar*) _src.c_str();
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint isCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

    if (isCompiled == GL_FALSE) {
        GLint infoLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 1) {
            std::vector<GLchar> infoLog(infoLength);
            glGetShaderInfoLog(shader, infoLength, NULL, &infoLog[0]);
            logMsg("Error compiling shader:\n%s\n", &infoLog[0]);
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;

}
