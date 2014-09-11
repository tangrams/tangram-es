#pragma once

#include "platform.h"
#include <string>
#include <vector>
#include <unordered_map>

/*
 * ShaderProgram - utility class representing an OpenGL shader program
 */

class ShaderProgram {

public:

    ShaderProgram();
    virtual ~ShaderProgram();

    /* Getters */
    GLint getGlProgram() { return m_glProgram; };
    GLint getGlFragmentShader() { return m_glFragmentShader; };
    GLint getGlVertexShader() { return m_glVertexShader; };

    /*
     * getAttribLocation - fetches the location of a shader attribute, caching the result
     */
    GLint getAttribLocation(const std::string& _attribName);

    /*
     * buildFromSourceStrings - attempts to compile a fragment shader and vertex shader from
     * strings representing the source code for each, then links them into a complete program;
     * if compiling or linking fails it prints the compiler log, returns false, and keeps the
     * program's previous state; if successful it returns true.
     */
    bool buildFromSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc);

    // TODO: Once we have file system abstractions, provide a method to build a program from file names

    /*
     * isValid - returns true if this object represents a valid OpenGL shader program
     */
    bool isValid() { return m_glProgram != 0; };

    /*
     * use - binds the program in openGL if it is not already bound.
     */
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
