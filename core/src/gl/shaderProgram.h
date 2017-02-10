#pragma once

#include "gl.h"
#include "gl/disposer.h"
#include "gl/shaderSource.h"
#include "gl/uniform.h"
#include "util/fastmap.h"

#include "glm/glm.hpp"

#include <string>
#include <vector>
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

    void setShaderSource(const std::string& _vertSrc, const std::string& _fragSrc) {
        m_fragmentShaderSource = _fragSrc;
        m_vertexShaderSource = _vertSrc;
        m_needsBuild = true;
    }

    // Apply all source blocks to the source strings for this shader and attempt to compile
    // and then link the resulting vertex and fragment shaders; if compiling or linking fails
    // this prints the compiler log, returns false, and keeps the program's previous state; if
    // successful it returns true.
    bool build(RenderState& rs);

    // Getters
    GLuint getGlProgram() const { return m_glProgram; };

    std::string getDescription() const { return m_description; }

    // Fetch the location of a shader attribute, caching the result.
    GLint getAttribLocation(const std::string& _attribName);

    // Fetch the location of a shader uniform, caching the result.
    GLint getUniformLocation(const UniformLocation& _uniformName);

    // Return true if this object represents a valid OpenGL shader program.
    bool isValid() const { return m_glProgram != 0; };

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

    void setDescription(std::string _description) { m_description = _description; }

    static GLuint makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader);
    static GLuint makeCompiledShader(RenderState& rs, const std::string& _src, GLenum _type);

    const std::string& vertexShaderSource() { return m_vertexShaderSource; }
    const std::string& fragmentShaderSource() { return m_fragmentShaderSource; }

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

    GLuint m_glProgram = 0;

    fastmap<std::string, GLint> m_attribMap;
    fastmap<GLint, UniformValue> m_uniformCache;

    std::string m_fragmentShaderSource;
    std::string m_vertexShaderSource;

    // An optional shader description printed on compile failure
    std::string m_description;

    bool m_needsBuild = true;

    Disposer m_disposer;

};

}
