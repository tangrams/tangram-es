#pragma once

#include "platform.h"
#include "gl.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "texture.h"

#include "glm/glm.hpp"

#include "util/texture.h"
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

    /* Set the vertex and fragment shader GLSL source to the given strings */
    void setSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc);

    /*  Add a block of GLSL to be injected at "#pragma tangram: [_tagName]" in the shader sources */
    void addSourceBlock(const std::string& _tagName, const std::string& _glslSource, bool _allowDuplicate = true);

    /*  Remove a block of GLSL to be injected at "#pragma tangram: [_tagName]" in the shader sources */
    void removeSourceBlock(const std::string& _tagName, const std::string& _glslSource);

    /*
     * Applies all source blocks to the source strings for this shader and attempts to compile
     * and then link the resulting vertex and fragment shaders; if compiling or linking fails 
     * it prints the compiler log, returns false, and keeps the program's previous state; if 
     * successful it returns true.
     */
    bool build();

    /* Getters */
    const GLuint getGlProgram() const { return m_glProgram; };
    const GLuint getGlFragmentShader() const { return m_glFragmentShader; };
    const GLuint getGlVertexShader() const { return m_glVertexShader; };

    /*
     * Fetches the location of a shader attribute, caching the result
     */
    const GLint getAttribLocation(const std::string& _attribName);

    /*
     * Fetches the location of a shader uniform, caching the result
     */
    const GLint getUniformLocation(const std::string& _uniformName);

    /* 
     * Returns true if this object represents a valid OpenGL shader program
     */
    bool isValid() const { return m_glProgram != 0; };

    /* 
     * Binds the program in openGL if it is not already bound; If the shader sources
     * have been modified since the last time build() was called, also calls build()
     */
    void use();

    /* 
     * Ensures the program is bound and then sets the named uniform to the given value(s)
     */
    void setUniformi(const std::string& _name, int _value);
    void setUniformi(const std::string& _name, int _value0, int _value1);
    void setUniformi(const std::string& _name, int _value0, int _value1, int _value2);
    void setUniformi(const std::string& _name, int _value0, int _value1, int _value2, int _value3);

    void setUniformf(const std::string& _name, float _value);
    void setUniformf(const std::string& _name, float _value0, float _value1);
    void setUniformf(const std::string& _name, float _value0, float _value1, float _value2);
    void setUniformf(const std::string& _name, float _value0, float _value1, float _value2, float _value3);

    void setUniformf(const std::string& _name, const glm::vec2& _value){setUniformf(_name,_value.x,_value.y);}
    void setUniformf(const std::string& _name, const glm::vec3& _value){setUniformf(_name,_value.x,_value.y,_value.z);}
    void setUniformf(const std::string& _name, const glm::vec4& _value){setUniformf(_name,_value.x,_value.y,_value.z,_value.w);}

    /* 
     * Ensures the program is bound and then sets the named uniform to the values
     * beginning at the pointer _value; 4 values are used for a 2x2 matrix, 9 values for a 3x3, etc.
     */
    void setUniformMatrix2f(const std::string& _name, const float* _value, bool transpose = false);
    void setUniformMatrix3f(const std::string& _name, const float* _value, bool transpose = false);
    void setUniformMatrix4f(const std::string& _name, const float* _value, bool transpose = false);
    
    /* Invalidates all managed ShaderPrograms
     * 
     * This should be called in the event of a GL context loss; former GL shader object
     * handles are invalidated and immediately recreated.
     */
    static void invalidateAllPrograms();

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
    
    static GLuint s_activeGlProgram;
    static int s_validGeneration; // Incremented when GL context is invalidated
    
    int m_generation;
    GLuint m_glProgram;
    GLuint m_glFragmentShader;
    GLuint m_glVertexShader;
    std::unordered_map<std::string, ShaderLocation> m_attribMap;
    std::unordered_map<std::string, ShaderLocation> m_uniformMap;
    std::string m_fragmentShaderSource;
    std::string m_vertexShaderSource;
    
    std::map<std::string, std::vector<std::string>> m_sourceBlocks;
    
    bool m_needsBuild;
    
    void checkValidity();
    GLuint makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader);
    GLuint makeCompiledShader(const std::string& _src, GLenum _type);
    
    void applySourceBlocks(std::string& _vertSrcOut, std::string& _fragSrcOut);
    
    std::unordered_map<std::string, Texture::TextureSlot> m_textureSlots;
    GLuint m_freeTextureUnit;
    
};
