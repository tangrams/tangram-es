#pragma once

#include "gl.h"

#include <vector>
#include <queue>
#include <memory>
#include <string>

namespace Tangram {

struct TextureFiltering {
    GLenum m_min;
    GLenum m_mag;
};

struct TextureWrapping {
    GLenum m_wraps;
    GLenum m_wrapt;
};

struct TextureOptions {
    GLenum m_internalFormat;
    GLenum m_format;
    TextureFiltering m_filtering;
    TextureWrapping m_wrapping;
};

#define TANGRAM_MAX_TEXTURE_UNIT 6

class Texture {

public:

    Texture(unsigned int _width, unsigned int _height,
            TextureOptions _options = {GL_ALPHA, GL_ALPHA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}},
            bool _generateMipmaps = false);
    
    Texture(const std::string& _file,
            TextureOptions _options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}},
            bool _generateMipmaps = false);

    virtual ~Texture();

    /* Perform texture updates, should be called at least once and after adding data or resizing */
    virtual void update(GLuint _textureSlot);

    /* Resize the texture */
    void resize(const unsigned int _width, const unsigned int _height);

    /* Width and Height texture getters */
    unsigned int getWidth() const { return m_width; }
    unsigned int getHeight() const { return m_height; }
    
    void bind(GLuint _unit);

    GLuint getGlHandle() { return m_glHandle; }

    /* Sets texture data
     *
     * Has less priority than set sub data
     */
    void setData(const GLuint* _data, unsigned int _dataSize);

    /* Update a region of the texture */
    void setSubData(const GLuint* _subData, unsigned int _xoff, unsigned int _yoff, unsigned int _width, unsigned int _height);

    typedef std::pair<GLuint, GLuint> TextureSlot;

    static void invalidateAllTextures();
    
protected:
    void generate(GLuint _textureUnit);
    void checkValidity();

    TextureOptions m_options;
    std::vector<GLuint> m_data;
    GLuint m_glHandle;

    bool m_dirty;
    bool m_shouldResize;

    unsigned int m_width;
    unsigned int m_height;

    GLenum m_target;

    int m_generation;
    static int s_validGeneration;

private:
    struct TextureSubData {
        std::vector<GLuint> m_data;
        unsigned int m_xoff;
        unsigned int m_yoff;
        unsigned int m_width;
        unsigned int m_height;
    };

    size_t bytesPerPixel();

    bool m_generateMipmaps;

    // used to queue the subdata updates, each call of setSubData would be treated in the order that they arrived
    std::queue<TextureSubData> m_subData;
};

}
