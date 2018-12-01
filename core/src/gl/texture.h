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
    float displayScale = 1.f; // 0.5 for a "@2x" image.
    bool generateMipmaps = false;
};

class Texture {

public:

    explicit Texture(TextureOptions _options);

    Texture(const uint8_t* data, size_t length, TextureOptions _options);

    Texture(const Texture& _other) = delete;
    Texture& operator=(const Texture& _other) = delete;
    Texture(Texture&& _other) = delete;
    Texture& operator=(Texture&& _other) = delete;

    virtual ~Texture();

    bool loadImageFromMemory(const uint8_t* data, size_t length);

    /* Sets texture pixel data */
    bool setPixelData(int _width, int _height, int _bytesPerPixel, const GLubyte* _data, size_t _length);

    void setSpriteAtlas(std::unique_ptr<SpriteAtlas> sprites);

    /* Resize the texture */
    void resize(int width, int height);

    virtual bool bind(RenderState& rs, GLuint _unit);

    /* Width and Height texture getters */
    int width() const { return m_width; }
    int height() const { return m_height; }

    GLuint glHandle() const { return m_glHandle; }

    float displayScale() const { return m_options.displayScale; }

    const auto& spriteAtlas() const { return m_spriteAtlas; }

    /* Checks whether the texture has valid data and has been successfully uploaded to GPU */
    bool isValid() const;

    size_t bufferSize() const { return m_bufferSize; }

protected:

    // Bytes per pixel for current PixelFormat options
    size_t bpp() const;

    void generate(RenderState& rs, GLuint _textureUnit);

    bool upload(RenderState& rs, GLuint _textureUnit);

    bool sanityCheck(size_t _width, size_t _height, size_t _bytesPerPixel, size_t _length) const;

    void freeBufferData() {
        std::free(m_buffer);
        m_buffer = nullptr;
    }
    void setBufferData(GLubyte* buffer, size_t size) {
        if (m_buffer == buffer) { return; }
        std::free(m_buffer);
        m_buffer = buffer;
    }

    TextureOptions m_options;

    GLubyte* m_buffer = nullptr;
    size_t m_bufferSize = 0;

    GLuint m_glHandle = 0;

    bool m_shouldResize = false;
    // Dipose buffer after texture upload
    bool m_disposeBuffer = true;

    int m_width = 0;
    int m_height = 0;

    RenderState* m_rs = nullptr;

private:

    std::unique_ptr<SpriteAtlas> m_spriteAtlas;

};

} // namespace Tangram
