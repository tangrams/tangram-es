#pragma once

#include "gl.h"
#include "gl/disposer.h"

#include <vector>
#include <memory>
#include <string>

namespace Tangram {

class RenderState;

struct TextureFiltering {
    GLenum min;
    GLenum mag;
};

struct TextureWrapping {
    GLenum wraps;
    GLenum wrapt;
};

struct TextureOptions {
    GLenum internalFormat;
    GLenum format;
    TextureFiltering filtering;
    TextureWrapping wrapping;
};

#define DEFAULT_TEXTURE_OPTION \
    {GL_ALPHA, GL_ALPHA, \
    {GL_LINEAR, GL_LINEAR}, \
    {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}}

class Texture {

public:

    Texture(unsigned int _width, unsigned int _height,
            TextureOptions _options = DEFAULT_TEXTURE_OPTION,
            bool _generateMipmaps = false);

    Texture(const std::vector<char>& _data,
            TextureOptions _options = DEFAULT_TEXTURE_OPTION,
            bool _generateMipmaps = false);

    Texture(Texture&& _other);
    Texture& operator=(Texture&& _other);

    virtual ~Texture();

    /* Perform texture updates, should be called at least once and after adding data or resizing */
    virtual void update(RenderState& rs, GLuint _textureSlot);

    virtual void update(RenderState& rs, GLuint _textureSlot, const GLuint* data);

    /* Resize the texture */
    void resize(const unsigned int _width, const unsigned int _height);

    /* Width and Height texture getters */
    unsigned int getWidth() const { return m_width; }
    unsigned int getHeight() const { return m_height; }

    void bind(RenderState& rs, GLuint _unit);

    void setDirty(size_t yOffset, size_t height);

    GLuint getGlHandle() { return m_glHandle; }

    /* Sets texture data
     *
     * Has less priority than set sub data
     */
    void setData(const GLuint* _data, unsigned int _dataSize);

    /* Update a region of the texture */
    void setSubData(const GLuint* _subData, uint16_t _xoff, uint16_t _yoff,
                    uint16_t _width, uint16_t _height, uint16_t _stride);

    /* Checks whether the texture has valid data and has been successfully uploaded to GPU */
    bool isValid() const;

    typedef std::pair<GLuint, GLuint> TextureSlot;

    static void invalidateAllTextures();

    static bool isRepeatWrapping(TextureWrapping _wrapping);

    bool loadImageFromMemory(const std::vector<char>& _data);

    static void flipImageData(unsigned char *result, int w, int h, int depth);
    static void flipImageData(GLuint *result, int w, int h);

protected:

    void generate(RenderState& rs, GLuint _textureUnit);

    TextureOptions m_options;
    std::vector<GLuint> m_data;
    GLuint m_glHandle;

    struct DirtyRange {
        size_t min;
        size_t max;
    };
    std::vector<DirtyRange> m_dirtyRanges;

    bool m_shouldResize;

    unsigned int m_width;
    unsigned int m_height;

    GLenum m_target;

    Disposer m_disposer;

private:

    size_t bytesPerPixel();

    bool m_generateMipmaps;
};

}
