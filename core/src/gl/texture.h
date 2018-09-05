#pragma once

#include "gl.h"
#include "scene/spriteAtlas.h"

#include <vector>
#include <memory>
#include <string>

namespace Tangram {

class RenderState;

enum class TextureMinFilter : GLenum {
    NEAREST = GL_NEAREST,
    LINEAR = GL_LINEAR,
    NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR,
};

enum class TextureMagFilter : GLenum {
    NEAREST = GL_NEAREST,
    LINEAR = GL_LINEAR,
};

enum class TextureWrap : GLenum {
    CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
    REPEAT = GL_REPEAT,
};

enum class PixelFormat : GLenum {
    ALPHA = GL_ALPHA,
    LUMINANCE = GL_LUMINANCE,
    LUMINANCE_ALPHA = GL_LUMINANCE_ALPHA,
    RGB = GL_RGB,
    RGBA = GL_RGBA,
};

struct TextureOptions {
    TextureMinFilter minFilter = TextureMinFilter::LINEAR;
    TextureMagFilter magFilter = TextureMagFilter::LINEAR;
    TextureWrap wrapS = TextureWrap::CLAMP_TO_EDGE;
    TextureWrap wrapT = TextureWrap::CLAMP_TO_EDGE;
    PixelFormat pixelFormat = PixelFormat::RGBA;
    float density = 1.f;
    bool generateMipmaps = false;
};

class Texture {

public:

    Texture(unsigned int _width, unsigned int _height, TextureOptions _options);

    Texture(const std::vector<char>& _data, TextureOptions _options);

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

    bool loadImageFromMemory(const std::vector<char>& _data);

    static void flipImageData(unsigned char *result, int w, int h, int depth);
    static void flipImageData(GLuint *result, int w, int h);

    size_t bytesPerPixel();
    size_t bufferSize();

    auto& spriteAtlas() { return m_spriteAtlas; }
    const auto& spriteAtlas() const { return m_spriteAtlas; }

    float density() const { return m_options.density; }

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

    RenderState* m_rs = nullptr;

private:

    std::unique_ptr<SpriteAtlas> m_spriteAtlas;

};

}
