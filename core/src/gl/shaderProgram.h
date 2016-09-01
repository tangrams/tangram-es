#pragma once

#include "gl.h"
#include "gl/disposer.h"
#include "uniform.h"
#include "util/fastmap.h"

#include "glm/glm.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Tangram {

class RenderState;

//
// ShaderProgram - utility class representing an OpenGL shader program
//
class ShaderProgram {

public:

    ShaderProgram();
    ~ShaderProgram();

    // Set the vertex and fragment shader GLSL source to the given strings/
    void setSourceStrings(const std::string& _fragSrc, const std::string& _vertSrc);

    // Add a block of GLSL to be injected at "#pragma tangram: [_tagName]" in the shader sources.
    void addSourceBlock(const std::string& _tagName, const std::string& _glslSource, bool _allowDuplicate = true);

    // Apply all source blocks to the source strings for this shader and attempt to compile
    // and then link the resulting vertex and fragment shaders; if compiling or linking fails
    // this prints the compiler log, returns false, and keeps the program's previous state; if
    // successful it returns true.
    bool build(RenderState& rs);

    // Getters
    GLuint getGlProgram() const { return m_glProgram; };
    GLuint getGlFragmentShader() const { return m_glFragmentShader; };
    GLuint getGlVertexShader() const { return m_glVertexShader; };

    const std::string& getFragmentShaderSource() const { return m_fragmentShaderSource; }
    const std::string& getVertexShaderSource() const { return m_vertexShaderSource; }

    // Fetch the location of a shader attribute, caching the result.
    GLint getAttribLocation(const std::string& _attribName);

    // Fetch the location of a shader uniform, caching the result.
    GLint getUniformLocation(const UniformLocation& _uniformName);

    // Return true if this object represents a valid OpenGL shader program.
    bool isValid(RenderState& rs) const { return m_glProgram != 0; };

    // Bind the program in OpenGL if it is not already bound; If the shader sources
    // have been modified since the last time build() was called, also calls build().
    // Returns true if shader can be used (i.e. is valid).
    bool use(RenderState& rs);

    // Ensure the program is bound and then set the named uniform to the given value(s).
    void setUniformi(RenderState& rs, const UniformLocation& _loc, int _value);
    void setUniformi(RenderState& rs, const UniformLocation& _loc, int _value0, int _value1);
    void setUniformi(RenderState& rs, const UniformLocation& _loc, int _value0, int _value1, int _value2);
    void setUniformi(RenderState& rs, const UniformLocation& _loc, int _value0, int _value1, int _value2, int _value3);

    void setUniformf(RenderState& rs, const UniformLocation& _loc, float _value);
    void setUniformf(RenderState& rs, const UniformLocation& _loc, float _value0, float _value1);
    void setUniformf(RenderState& rs, const UniformLocation& _loc, float _value0, float _value1, float _value2);
    void setUniformf(RenderState& rs, const UniformLocation& _loc, float _value0, float _value1, float _value2, float _value3);

    void setUniformf(RenderState& rs, const UniformLocation& _loc, const glm::vec2& _value);
    void setUniformf(RenderState& rs, const UniformLocation& _loc, const glm::vec3& _value);
    void setUniformf(RenderState& rs, const UniformLocation& _loc, const glm::vec4& _value);

    void setUniformf(RenderState& rs, const UniformLocation& _loc, const UniformArray1f& _value);
    void setUniformf(RenderState& rs, const UniformLocation& _loc, const UniformArray2f& _value);
    void setUniformf(RenderState& rs, const UniformLocation& _loc, const UniformArray3f& _value);
    void setUniformi(RenderState& rs, const UniformLocation& _loc, const UniformTextureArray& _value);

    // Ensure the program is bound and then set the named uniform to the values
    // beginning at the pointer _value; 4 values are used for a 2x2 matrix, 9 values for a 3x3, etc.
    void setUniformMatrix2f(RenderState& rs, const UniformLocation& _loc, const glm::mat2& _value, bool transpose = false);
    void setUniformMatrix3f(RenderState& rs, const UniformLocation& _loc, const glm::mat3& _value, bool transpose = false);
    void setUniformMatrix4f(RenderState& rs, const UniformLocation& _loc, const glm::mat4& _value, bool transpose = false);

    static std::string getExtensionDeclaration(const std::string& _extension);

    auto getSourceBlocks() const { return  m_sourceBlocks; }

    void setDescription(std::string _description) { m_description = _description; }

    static std::string shaderSourceBlock(const unsigned char* data, size_t size) {
        std::string block;
        if (data[size - 1] == '\n') {
            block.append(reinterpret_cast<const char*>(data), size);
        } else {
            block.reserve(size + 2);
            block.append(reinterpret_cast<const char*>(data), size);
            block += '\n';
        }
        return block;
    }

private:

    // Get a uniform value from the cache, and returns false when it's a cache miss
    template <class T>
    inline bool getFromCache(GLint _location, T _value) {
        auto& v = m_uniformCache[_location];
        if (v.is<T>()) {
            T& value = v.get<T>();
            if (value == _value) {
                return true;
            }
        }
        v = _value;
        return false;
    }

    int m_generation = -1;
    GLuint m_glProgram = 0;
    GLuint m_glFragmentShader = 0;
    GLuint m_glVertexShader = 0;

    fastmap<std::string, GLint> m_attribMap;
    fastmap<GLint, UniformValue> m_uniformCache;

    std::string m_fragmentShaderSource;
    std::string m_vertexShaderSource;

    // An optional shader description printed on compile failure
    std::string m_description;

    std::map<std::string, std::vector<std::string>> m_sourceBlocks;

    bool m_needsBuild = true;
    bool m_invalidShaderSource = false;

    Disposer m_disposer;

    void checkValidity(RenderState& rs);
    GLuint makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader);
    GLuint makeCompiledShader(const std::string& _src, GLenum _type);

    std::string applySourceBlocks(const std::string& source, bool fragShader);

};

#define SHADER_SOURCE(NAME) ShaderProgram::shaderSourceBlock(NAME ## _data, NAME ## _size)

}
