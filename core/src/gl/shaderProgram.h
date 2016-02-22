#pragma once

#include "gl.h"
#include "glm/glm.hpp"
#include "util/fastmap.h"
#include "util/uniform.h"

#include <string>
#include <vector>
#include <map>

namespace Tangram {
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

    /*
     * Applies all source blocks to the source strings for this shader and attempts to compile
     * and then link the resulting vertex and fragment shaders; if compiling or linking fails
     * it prints the compiler log, returns false, and keeps the program's previous state; if
     * successful it returns true.
     */
    bool build();

    /* Getters */
    GLuint getGlProgram() const { return m_glProgram; };
    GLuint getGlFragmentShader() const { return m_glFragmentShader; };
    GLuint getGlVertexShader() const { return m_glVertexShader; };

    /*
     * Fetches the location of a shader attribute, caching the result
     */
    GLint getAttribLocation(const std::string& _attribName);

    /*
     * Fetches the location of a shader uniform, caching the result
     */
    GLint getUniformLocation(const std::string& _uniformName);

    /*
     * Returns true if this object represents a valid OpenGL shader program
     */
    bool isValid() const { return m_glProgram != 0; };

    /*
     * Binds the program in openGL if it is not already bound; If the shader sources
     * have been modified since the last time build() was called, also calls build().
     * Returns true if shader can be used (i.e. is valid)
     */
    bool use();

    /*
     * Ensures the program is bound and then sets the named uniform to the given value(s)
     * Note: _name shoud have the same address for consistent uniform location caching
     */
    void setUniformi(const std::string& _name, int _value);
    void setUniformi(const std::string& _name, int _value0, int _value1);
    void setUniformi(const std::string& _name, int _value0, int _value1, int _value2);
    void setUniformi(const std::string& _name, int _value0, int _value1, int _value2, int _value3);

    void setUniformf(const std::string& _name, float _value);
    void setUniformf(const std::string& _name, float _value0, float _value1);
    void setUniformf(const std::string& _name, float _value0, float _value1, float _value2);
    void setUniformf(const std::string& _name, float _value0, float _value1, float _value2, float _value3);

    void setUniformf(const std::string& _name, const glm::vec2& _value);
    void setUniformf(const std::string& _name, const glm::vec3& _value);
    void setUniformf(const std::string& _name, const glm::vec4& _value);

    /*
     * Ensures the program is bound and then sets the named uniform to the values
     * beginning at the pointer _value; 4 values are used for a 2x2 matrix, 9 values for a 3x3, etc.
     */
    void setUniformMatrix2f(const std::string& _name, const glm::mat2& _value, bool transpose = false);
    void setUniformMatrix3f(const std::string& _name, const glm::mat3& _value, bool transpose = false);
    void setUniformMatrix4f(const std::string& _name, const glm::mat4& _value, bool transpose = false);

    /* Invalidates all managed ShaderPrograms
     *
     * This should be called in the event of a GL context loss; former GL shader object
     * handles are invalidated and immediately recreated.
     */
    static void invalidateAllPrograms();

    auto getSourceBlocks() const { return  m_sourceBlocks; }

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

    static int s_validGeneration; // Incremented when GL context is invalidated

    // Get a uniform value from the cache, and returns false when it's a cache miss
    template <class T>
    inline bool getFromCache(GLint _location, T _value) {
        const auto& v = m_uniformCache.find(_location);
        bool cached = false;
        if (v != m_uniformCache.end()) {
            if (v->second.is<T>()) {
                T& value = v->second.get<T>();
                cached = value == _value;
                if (!cached) {
                    value = _value;
                }
            }
        } else { m_uniformCache[_location] = _value; }
        return cached;
    }

    int m_generation;
    GLuint m_glProgram;
    GLuint m_glFragmentShader;
    GLuint m_glVertexShader;

    fastmap<std::string, ShaderLocation> m_attribMap;
    fastmap<intptr_t, ShaderLocation> m_uniformMap;
    fastmap<GLint, UniformValue> m_uniformCache;

    std::string m_fragmentShaderSource;
    std::string m_vertexShaderSource;

    std::map<std::string, std::vector<std::string>> m_sourceBlocks;

    bool m_needsBuild;
    bool m_invalidShaderSource;

    void checkValidity();
    GLuint makeLinkedShaderProgram(GLint _fragShader, GLint _vertShader);
    GLuint makeCompiledShader(const std::string& _src, GLenum _type);

    std::string applySourceBlocks(const std::string& source, bool fragShader);

};

namespace Uniform {
    extern const std::string time;
    extern const std::string model;
    extern const std::string tileOrigin;
    extern const std::string devicePixelRatio;
    extern const std::string resolution;
    extern const std::string mapPosition;
    extern const std::string normalMatrix;
    extern const std::string inverseNormalMatrix;
    extern const std::string metersPerPixel;
    extern const std::string view;
    extern const std::string proj;
    extern const std::string ortho;
    extern const std::string tex;
    extern const std::string uvScaleFactor;
    extern const std::string materialEmission;
    extern const std::string materialEmissionTexture;
    extern const std::string materialEmissionScale;
    extern const std::string materialAmbiant;
    extern const std::string materialAmbiantTexture;
    extern const std::string materialAmbiantScale;
    extern const std::string materialDiffuse;
    extern const std::string materialDiffuseTexture;
    extern const std::string materialDiffuseScale;
    extern const std::string materialShininess;
    extern const std::string materialSpecular;
    extern const std::string materialSpecularTexture;
    extern const std::string materialSpecularScale;
    extern const std::string materialNormalTexture;
    extern const std::string materialNormalScale;
    extern const std::string materialNormalAmount;
};

}
