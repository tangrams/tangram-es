#pragma once

#include "platform.h"
#include "error.h"
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
    GLuint getGlProgram() const { return m_glProgram; };
    GLuint getGlFragmentShader() const { return m_glFragmentShader; };
    GLuint getGlVertexShader() const { return m_glVertexShader; };

    /*
     * getAttribLocation - fetches the location of a shader attribute, caching the result
     */
    GLint getAttribLocation(const std::string& _attribName);

    /*
     * getUniformLocation - fetches the location of a shader uniform, caching the result
     */
    GLint getUniformLocation(const std::string& _uniformName);

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
    bool isValid() const { return m_glProgram == 0; };

    /*
     * use - binds the program in openGL if it is not already bound.
     */
    void use() const;

private:

    struct ShaderLocation {
        GLint loc;
        ShaderLocation() : loc(-2) {}
        // This struct exists to resolve an ambiguity in shader locations:
        // In the unordered_maps that store shader uniform and attrib locations,
        // Un-mapped 'keys' are initialized by constructing the 'value' type and
        // for numerical types this constructs a value of 0. But 0 is a valid 
        // location, so it is ambiguous whether the value is unmapped or simply 0.
        // Therefore, we use a dummy structure which does nothing but initialize
        // to a value that is not a valid uniform or attribute location. 
    };

    static GLint s_activeGlProgram;

    GLuint m_glProgram;
    GLuint m_glFragmentShader;
    GLuint m_glVertexShader;
    std::unordered_map<std::string, ShaderLocation> m_attribMap;
    std::unordered_map<std::string, ShaderLocation> m_uniformMap;
    std::string m_fragmentShaderSource;
    std::string m_vertexShaderSource;

    GLuint makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader);
    GLuint makeCompiledShader(const std::string& _src, GLenum _type);

};
