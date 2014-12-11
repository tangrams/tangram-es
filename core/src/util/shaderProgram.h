#pragma once

#include "platform.h"
#include "gl.h"
#include "error.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

#include "glm/glm.hpp"

// TODO:
// each ShaderProgram instance has a map of <string, vector<string>> pairs
// the string identifies the tag to replace, the vector is a list of strings of GLSL to inject
// the ShaderProgram class also has a static map of <string, vector<string>> pairs, that are injected in ALL program instances
// class-level blocks are injected before instance-level blocks

/*
 * ShaderProgram - utility class representing an OpenGL shader program
 */

class ShaderProgram {
public:

    ShaderProgram();
    virtual ~ShaderProgram();

    /*  Load the string code for fragment and vertex shaders*/
    void loadSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc);

    /*  Add a key to replace every "#prama tangram: [tagName]" for the strings added */
    void addBlock(const std::string& _tagName, const std::string& _glslSource);

    /*  Parser throught vert and frag shader replacing every "#prama tangram: [tagName]" 
    * using the m_blocks key and strings */
    bool build();

    /* Getters */
    const GLuint getGlProgram() const { return m_glProgram; };
    const GLuint getGlFragmentShader() const { return m_glFragmentShader; };
    const GLuint getGlVertexShader() const { return m_glVertexShader; };

    /*
     * getAttribLocation - fetches the location of a shader attribute, caching the result
     */
    const GLint getAttribLocation(const std::string& _attribName);

    /*
     * getUniformLocation - fetches the location of a shader uniform, caching the result
     */
    const GLint getUniformLocation(const std::string& _uniformName);

    // TODO: Once we have file system abstractions, provide a method to build a program from file names

    /*
     * isValid - returns true if this object represents a valid OpenGL shader program
     */
    bool isValid() const { return m_glProgram != 0; };

    /*
     * use - binds the program in openGL if it is not already bound.
     */
    void use() const;

    /*
     * setUniform - ensures the program is bound and then sets the named uniform to the given value(s)
     */
    void setUniformi(const std::string& _name, int _value);
    void setUniformi(const std::string& _name, int _value0, int _value1);
    void setUniformi(const std::string& _name, int _value0, int _value1, int _value2);
    void setUniformi(const std::string& _name, int _value0, int _value1, int _value2, int _value3);

    void setUniformf(const std::string& _name, float _value);
    void setUniformf(const std::string& _name, float _value0, float _value1);
    void setUniformf(const std::string& _name, float _value0, float _value1, float _value2);
    void setUniformf(const std::string& _name, float _value0, float _value1, float _value2, float _value3);
    
    /*
     * setUniformMatrix - ensure the program is bound and then sets the named uniform to the values
     * beginning at the pointer _value; 4 values are used for a 2x2 matrix, 9 values for a 3x3, etc.
     */
    void setUniformMatrix2f(const std::string& _name, float* _value, bool transpose = false);
    void setUniformMatrix3f(const std::string& _name, float* _value, bool transpose = false);
    void setUniformMatrix4f(const std::string& _name, float* _value, bool transpose = false);
    
    /*
        WIP - start: This is need for fast lights &  Implementation
     */
    void setUniformf(const std::string& _name, glm::vec2 _value){setUniformf(_name,_value.x,_value.y);}
    void setUniformf(const std::string& _name, glm::vec3 _value){setUniformf(_name,_value.x,_value.y,_value.z);}
    void setUniformf(const std::string& _name, glm::vec4 _value){setUniformf(_name,_value.x,_value.y,_value.z,_value.w);}
    /* 
        WIP - end
     */

private:

    struct ShaderLocation {
        GLint loc;
        ShaderLocation() : loc(-2) {}
        // This struct exists to resolve an ambiguity in shader locations:
        // In the unordered_maps that store shader uniform and attrib locations,
        // Un-mapped 'keys' are initialized by constructing the 'value' type.
        // For numerical types this constructs a value of 0. But 0 is a valid 
        // location, so it is ambiguous whether the value is unmapped or simply 0.
        // Therefore, we use a dummy structure which does nothing but initialize
        // to a value that is not a valid uniform or attribute location. 
    };

    /*
     * buildFromSourceStrings - attempts to compile a fragment shader and vertex shader from
     * strings representing the source code for each, then links them into a complete program;
     * if compiling or linking fails it prints the compiler log, returns false, and keeps the
     * program's previous state; if successful it returns true.
     */
    bool buildFromSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc);

    GLuint makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader);
    GLuint makeCompiledShader(const std::string& _src, GLenum _type);

    std::unordered_map<std::string, ShaderLocation> m_attribMap;
    std::unordered_map<std::string, ShaderLocation> m_uniformMap;
    std::string m_fragmentShaderSource;
    std::string m_vertexShaderSource;

    static GLint s_activeGlProgram;

    GLuint m_glProgram;
    GLuint m_glFragmentShader;
    GLuint m_glVertexShader;

    std::map<std::string, std::vector<std::string>> m_blocks;
};
