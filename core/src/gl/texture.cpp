#include "gl/texture.h"

#include "gl/glError.h"
#include "gl/renderState.h"
#include "gl/hardware.h"
#include "log.h"
#include "map.h"
#include "platform.h"
#include "util/geom.h"

// Enable only JPEG, PNG, GIF, TGA and PSD
#define STBI_NO_BMP
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cassert>
#include <cstring> // for memset

namespace Tangram {

constexpr GLenum TEXTURE_TARGET = GL_TEXTURE_2D;

Texture::Texture(TextureOptions _options) : m_options(_options) {
}

Texture::Texture(const uint8_t* data, size_t length, TextureOptions options)
    : Texture(options) {
    loadImageFromMemory(data, length);
}

Texture::~Texture() {
    if (m_rs) {
        m_rs->queueTextureDeletion(m_glHandle);
    }
}

bool Texture::loadImageFromMemory(const uint8_t* data, size_t length) {
    // stbi_load_from_memory loads the image as a series of scanlines starting
    // from the top-left corner of the image. This flips the output such that
    // the data begins at the bottom-left corner, as required for our OpenGL
    // texture coordinates.
    stbi_set_flip_vertically_on_load(true);

    int width = 0, height = 0;
    int channelsInFile = 0;
    int channelsRequested = bpp();

    unsigned char* pixels = stbi_load_from_memory(data, static_cast<int>(length),
                                                  &width, &height, &channelsInFile,
                                                  channelsRequested);

    if (!pixels) {
        LOGE("Could not load image data: %dx%d bpp:%d/%d",
             width, height, channelsInFile, channelsRequested);

        // Default inconsistent texture data is set to a 1*1 pixel texture
        // This reduces inconsistent behavior when texture failed loading
        // texture data but a Tangram style shader requires a shader sampler
        GLubyte pixel[4] = { 0, 0, 0, 255 };
        setPixelData(1, 1, bpp(), pixel, bpp());
        return false;
    }

    bool ok = movePixelData(width, height, channelsRequested, pixels,
                            width * height * channelsRequested);
    if (!ok) {
        LOGE("Could not load image data: %dx%d bpp:%d/%d",
             width, height, channelsInFile, channelsRequested);
    }
    return ok;
}

bool Texture::movePixelData(int _width, int _height, int _bytesPerPixel,
                            GLubyte* _data, size_t _length) {

    if (!sanityCheck(_width, _height, _bytesPerPixel, _length)) {
        free(_data);
        return false;
    }

    m_buffer.reset(_data);
    m_bufferSize = _length;
    m_bytesPerPixel = _bytesPerPixel;

    resize(_width, _height);
    setRowsDirty(0, m_height);
    return true;
}

bool Texture::setPixelData(int _width, int _height, int _bytesPerPixel,
                           const GLubyte* _data, size_t _length) {

    if (!sanityCheck(_width, _height, _bytesPerPixel, _length)) {
        return false;
    }

    m_buffer.reset();
    m_bufferSize = 0;

    auto buffer = reinterpret_cast<GLubyte*>(std::malloc(_length));
    if (!buffer) {
        LOGE("Could not allocate texture: Out of memory!");
        return false;
    }
    std::memcpy(buffer, _data, _length);

    m_buffer.reset(buffer);
    m_bufferSize = _length;
    m_bytesPerPixel = _bytesPerPixel;

    resize(_width, _height);
    setRowsDirty(0, m_height);
    return true;
}

void Texture::setRowsDirty(int start, int count) {
    // FIXME: check that dirty range is valid for texture size!
    int max = start + count;
    int min = start;

    if (m_dirtyRows.empty()) {
        m_dirtyRows.push_back({min, max});
        return;
    }

    auto n = m_dirtyRows.begin();

    // Find first overlap
    while (n != m_dirtyRows.end()) {
        if (min > n->max) {
            // this range is after current
            ++n;
            continue;
        }
        if (max < n->min) {
            // this range is before current
            m_dirtyRows.insert(n, {min, max});
            return;
        }
        // Combine with overlapping range
        n->min = std::min(n->min, min);
        n->max = std::max(n->max, max);
        break;
    }
    if (n == m_dirtyRows.end()) {
        m_dirtyRows.push_back({min, max});
        return;
    }

    // Merge up to last overlap
    auto it = n+1;
    while (it != m_dirtyRows.end() && max >= it->min) {
        n->max = std::max(it->max, max);
        it = m_dirtyRows.erase(it);
    }
}

void Texture::setSpriteAtlas(std::unique_ptr<Tangram::SpriteAtlas> sprites) {
    m_spriteAtlas = std::move(sprites);
}

void Texture::generate(RenderState& _rs, GLuint _textureUnit) {
    GL::genTextures(1, &m_glHandle);

    _rs.texture(m_glHandle, _textureUnit, TEXTURE_TARGET);

    GL::texParameteri(TEXTURE_TARGET, GL_TEXTURE_MIN_FILTER,
                      static_cast<GLint>(m_options.minFilter));
    GL::texParameteri(TEXTURE_TARGET, GL_TEXTURE_MAG_FILTER,
                      static_cast<GLint>(m_options.magFilter));

    GL::texParameteri(TEXTURE_TARGET, GL_TEXTURE_WRAP_S,
                      static_cast<GLint>(m_options.wrapS));
    GL::texParameteri(TEXTURE_TARGET, GL_TEXTURE_WRAP_T,
                      static_cast<GLint>(m_options.wrapT));

    m_rs = &_rs;
}

bool Texture::isValid() const {
    return (m_glHandle != 0) || bool(m_buffer);
}

bool Texture::bind(RenderState& _rs, GLuint _textureUnit) {

    if (!m_shouldResize && m_dirtyRows.empty()) {
        if (m_glHandle == 0) { return false; }

        _rs.texture(m_glHandle, _textureUnit, TEXTURE_TARGET);
        return true;
    }

    if (m_shouldResize) {
        m_shouldResize = false;
        m_dirtyRows.clear();

        if (Hardware::maxTextureSize < m_width ||
            Hardware::maxTextureSize < m_height) {
            LOGW("Texture larger than Hardware maximum texture size");
            if (m_disposeBuffer) { m_buffer.reset(); }
            return false;
        }
        if (m_glHandle == 0) {
            generate(_rs, _textureUnit);
        } else {
            _rs.texture(m_glHandle, _textureUnit, TEXTURE_TARGET);
        }

        auto format = static_cast<GLenum>(m_options.pixelFormat);
        GL::texImage2D(TEXTURE_TARGET, 0, format, m_width, m_height, 0, format,
                       GL_UNSIGNED_BYTE, m_buffer.get());

        if (m_buffer && m_options.generateMipmaps) {
            GL::generateMipmap(TEXTURE_TARGET);
        }

        if (m_disposeBuffer) { m_buffer.reset(); }
        return true;
    }

    if (m_glHandle == 0) {
        LOGW("Texture is not ready!");
        return false;
    } else if (!m_buffer) {
        LOGE("No data to update Texture!");
        return false;
    }

    _rs.texture(m_glHandle, _textureUnit, TEXTURE_TARGET);

    auto format = static_cast<GLenum>(m_options.pixelFormat);
    for (auto& range : m_dirtyRows) {
        auto rows = range.max - range.min;
        auto offset = m_buffer.get() + (range.min * m_width * m_bytesPerPixel);
        GL::texSubImage2D(TEXTURE_TARGET, 0, 0, range.min, m_width, rows, format,
                          GL_UNSIGNED_BYTE, offset);
    }
    m_dirtyRows.clear();

    return true;
}

void Texture::resize(int width, int height) {
    assert(width >= 0);
    assert(height >= 0);
    m_width = width;
    m_height = height;

    if (!(Hardware::supportsTextureNPOT) &&
        !(isPowerOfTwo(m_width) && isPowerOfTwo(m_height)) &&
        (m_options.generateMipmaps || (m_options.wrapS == TextureWrap::REPEAT ||
                                       m_options.wrapT == TextureWrap::REPEAT))) {
        LOGW("OpenGL ES doesn't support texture repeat" \
             " wrapping for NPOT textures nor mipmap textures");
        LOGW("Falling back to LINEAR Filtering");
        m_options.minFilter =TextureMinFilter::LINEAR;
        m_options.magFilter = TextureMagFilter::LINEAR;
        m_options.generateMipmaps = false;
    }

    m_shouldResize = true;
    m_dirtyRows.clear();
}

size_t Texture::bpp() const {
    switch (m_options.pixelFormat) {
    case PixelFormat::ALPHA:
    case PixelFormat::LUMINANCE:
        return 1;
    case PixelFormat::LUMINANCE_ALPHA:
        return 2;
    case PixelFormat::RGB:
        return 3;
    default:
        return 4;
    }
}

bool Texture::sanityCheck(size_t _width, size_t _height, size_t _bytesPerPixel,
                          size_t _length) const {
    size_t dim = _width * _height;
    if (_length != dim * bpp()) {
        LOGW("Invalid data size for Texture dimension! %dx%d bpp:%d bytes:%d",
             _width, _height, _bytesPerPixel, _length);
        return false;
    }
    if (bpp() != _bytesPerPixel) {
        LOGW("PixelFormat and bytesPerPixel do not match! %d:%d",
             bpp(), _bytesPerPixel);
        return false;
    }
    return true;
}

}
