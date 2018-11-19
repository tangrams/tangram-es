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

Texture::Texture(const uint8_t* data, size_t length, TextureOptions options) : Texture(options) {
    loadImageFromMemory(data, length);
}

Texture::~Texture() {
    if (m_rs) {
        m_rs->queueTextureDeletion(m_glHandle);
    }
}

bool Texture::loadImageFromMemory(const uint8_t* data, size_t length) {
    // stbi_load_from_memory loads the image as a series of scanlines starting from
    // the top-left corner of the image. This flips the output such that the data
    // begins at the bottom-left corner, as required for our OpenGL texture coordinates.
    stbi_set_flip_vertically_on_load(true);

    int width = 0, height = 0;
    int channelsInFile = 0;
    int channelsRequested = getBytesPerPixel();

    unsigned char* pixels = stbi_load_from_memory(data, static_cast<int>(length),
                                                  &width, &height, &channelsInFile,
                                                  channelsRequested);

    if (pixels) {
        movePixelData(width, height, channelsRequested, pixels,
                      width * height * channelsRequested);

        return true;
    }
    // Default inconsistent texture data is set to a 1*1 pixel texture
    // This reduces inconsistent behavior when texture failed loading
    // texture data but a Tangram style shader requires a shader sampler
    GLuint blackPixel = 0x00000ff;

    setPixelData(1, 1, sizeof(blackPixel), reinterpret_cast<GLubyte*>(&blackPixel), sizeof(blackPixel));

    return false;
}

Texture::Texture(Texture&& _other) noexcept {
    *this = std::move(_other);
}

Texture& Texture::operator=(Texture&& _other) noexcept {
    m_glHandle = _other.m_glHandle;
    _other.m_glHandle = 0;

    m_options = _other.m_options;
    m_data = std::move(_other.m_data);
    m_dirtyRows = std::move(_other.m_dirtyRows);
    m_shouldResize = _other.m_shouldResize;
    m_width = _other.m_width;
    m_height = _other.m_height;
    m_rs = _other.m_rs;

    return *this;
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

void Texture::bind(RenderState& rs, GLuint _unit) {
    rs.textureUnit(_unit);
    rs.texture(TEXTURE_TARGET, m_glHandle);
}

void Texture::generate(RenderState& rs, GLuint _textureUnit) {
    GL::genTextures(1, &m_glHandle);

    bind(rs, _textureUnit);

    GL::texParameteri(TEXTURE_TARGET, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(m_options.minFilter));
    GL::texParameteri(TEXTURE_TARGET, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(m_options.magFilter));

    GL::texParameteri(TEXTURE_TARGET, GL_TEXTURE_WRAP_S, static_cast<GLint>(m_options.wrapS));
    GL::texParameteri(TEXTURE_TARGET, GL_TEXTURE_WRAP_T, static_cast<GLint>(m_options.wrapT));

    m_rs = &rs;
}

bool Texture::isValid() const {
    return (m_glHandle != 0) || bool(m_buffer);
}

void Texture::update(RenderState& _rs, GLuint _textureUnit) {

    if (!m_shouldResize && m_dirtyRows.empty()) { return; }

    if (m_glHandle == 0 && !m_buffer) {
        auto buffer = reinterpret_cast<GLubyte*>(std::malloc(m_width * m_height * getBytesPerPixel()));
        if (!buffer) {
            LOGE("Could not allocate texture: Out of memory!");
            return;
        }
        m_buffer.reset(buffer);
    }

    update(_rs, _textureUnit, m_buffer.get());

    m_buffer.reset();
}

void Texture::update(RenderState& _rs, GLuint _textureUnit, const GLubyte* _buffer) {

    if (!m_shouldResize && m_dirtyRows.empty()) { return; }

    if (m_glHandle == 0) {
        // texture hasn't been initialized yet, generate it
        generate(_rs, _textureUnit);
    } else {
        bind(_rs, _textureUnit);
    }

    auto format = static_cast<GLenum>(m_options.pixelFormat);

    // resize or push data
    if (m_shouldResize) {
        if (Hardware::maxTextureSize < m_width || Hardware::maxTextureSize < m_height) {
            LOGW("The hardware maximum texture size is currently reached");
        }

        GL::texImage2D(TEXTURE_TARGET, 0, format, m_width, m_height, 0, format,
                       GL_UNSIGNED_BYTE, _buffer);

        if (m_buffer && m_options.generateMipmaps) {
            // generate the mipmaps for this texture
            GL::generateMipmap(TEXTURE_TARGET);
        }
        m_shouldResize = false;
    } else {
        for (auto& range : m_dirtyRows) {
            const GLubyte* offset = _buffer + (range.min * m_width * m_bytesPerPixel);

            GL::texSubImage2D(TEXTURE_TARGET, 0, 0, range.min, m_width, range.max - range.min,
                              format, GL_UNSIGNED_BYTE, offset);
        }
    }

    m_dirtyRows.clear();
}

void Texture::resize(int width, int height) {

    assert(width >= 0);
    assert(height >= 0);
    m_width = width;
    m_height = height;

    if (!(Hardware::supportsTextureNPOT) &&
        !(isPowerOfTwo(m_width) && isPowerOfTwo(m_height)) &&
        (m_options.generateMipmaps || (m_options.wrapS == TextureWrap::REPEAT || m_options.wrapT == TextureWrap::REPEAT))) {
        LOGW("OpenGL ES doesn't support texture repeat wrapping for NPOT textures nor mipmap textures");
        LOGW("Falling back to LINEAR Filtering");
        m_options.minFilter =TextureMinFilter::LINEAR;
        m_options.magFilter = TextureMagFilter::LINEAR;
        m_options.generateMipmaps = false;
    }

    m_shouldResize = true;
    m_dirtyRows.clear();
}

size_t Texture::getBytesPerPixel() const {
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

bool Texture::sanityCheck(size_t _width, size_t _height, size_t _bytesPerPixel, size_t _length) const {
    size_t dim = m_width * m_height;
    if (_length != dim * getBytesPerPixel()) {
        LOGE("Invalid data size for Texture dimension! %dx%d bpp:%d bytes:%d",
             _width, _height, _bytesPerPixel, _length);
        return false;
    }
    if (getBytesPerPixel() != _bytesPerPixel) {
        LOGE("PixelFormat and bytesPerPixel do not match! %d:%d",
             getBytesPerPixel(), _bytesPerPixel);
        return false;
    }
    return true;
}

void Texture::movePixelData(size_t _width, size_t _height, size_t _bytesPerPixel,
                            GLubyte* _data, size_t _length) {
    if (m_glHandle != 0) {
        free(_data);
        LOGE("Texture data has already been set!");
        return;
    }

    if (sanityCheck(_width, _height, _bytesPerPixel, _length)) {
        free(_data);
        return;
    }

    m_buffer.reset(_data);
    m_bufferSize = _length;
    m_bytesPerPixel = _bytesPerPixel;

    resize(_width, _height);
    setRowsDirty(0, m_height);
}

void Texture::setPixelData(size_t _width, size_t _height, size_t _bytesPerPixel,
                           const GLubyte* _data, size_t _length) {

    if (m_glHandle != 0) {
        LOGE("Texture data has already been set!");
        return;
    }
    if (sanityCheck(_width, _height, _bytesPerPixel, _length)) {
        return;
    }

    m_buffer.reset();
    m_bufferSize = 0;

    auto buffer = reinterpret_cast<GLubyte*>(std::malloc(_length));
    if (!buffer) {
        LOGE("Could not allocate texture: Out of memory!");
        return;
    }
    std::memcpy(buffer, _data, _length);

    m_buffer.reset(buffer);
    m_bufferSize = _length;
    m_bytesPerPixel = _bytesPerPixel;

    resize(_width, _height);
    setRowsDirty(0, m_height);
}

}
