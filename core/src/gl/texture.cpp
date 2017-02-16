#include "gl/texture.h"

#include "gl/error.h"
#include "gl/renderState.h"
#include "gl/hardware.h"
#include "util/geom.h"
#include "log.h"
#include "platform.h"
#include "tangram.h"

// Enable only JPEG, PNG, GIF, TGA and PSD
#define STBI_NO_BMP
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstring> // for memset

namespace Tangram {

Texture::Texture(unsigned int _width, unsigned int _height, TextureOptions _options, bool _generateMipmaps)
    : m_options(_options), m_generateMipmaps(_generateMipmaps) {

    m_glHandle = 0;
    m_shouldResize = false;
    m_target = GL_TEXTURE_2D;
    resize(_width, _height);
}

Texture::Texture(const std::vector<char>& _data, TextureOptions options, bool generateMipmaps)
    : Texture(0u, 0u, options, generateMipmaps) {

    loadImageFromMemory(_data);
}

Texture::~Texture() {

    auto glHandle = m_glHandle;
    auto target = m_target;

    m_disposer([=](RenderState& rs) {
        // If the currently-bound texture is deleted, the binding resets to 0
        // according to the OpenGL spec, so unset this texture binding.
        rs.textureUnset(target, glHandle);

        GL::deleteTextures(1, &glHandle);
    });
}

bool Texture::loadImageFromMemory(const std::vector<char>& _data) {
    unsigned char* pixels = nullptr;
    int width, height, comp;

    if (_data.size() != 0) {
        pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(_data.data()), _data.size(), &width, &height, &comp, STBI_rgb_alpha);
    }

    if (pixels) {
        // stbi_load_from_memory loads the image as a series of scanlines starting from
        // the top-left corner of the image. This call flips the output such that the data
        // begins at the bottom-left corner, as required for our OpenGL texture coordinates.
        auto* rgbaPixels = reinterpret_cast<GLuint*>(pixels);

        Texture::flipImageData(rgbaPixels, width, height);

        resize(width, height);

        setData(rgbaPixels, width * height);

        stbi_image_free(pixels);

        return true;
    }
    // Default inconsistent texture data is set to a 1*1 pixel texture
    // This reduces inconsistent behavior when texture failed loading
    // texture data but a Tangram style shader requires a shader sampler
    GLuint blackPixel = 0x0000ff;

    setData(&blackPixel, 1);

    return false;
}

Texture::Texture(Texture&& _other) {
    *this = std::move(_other);
}

Texture& Texture::operator=(Texture&& _other) {
    m_glHandle = _other.m_glHandle;
    _other.m_glHandle = 0;

    m_options = _other.m_options;
    m_data = std::move(_other.m_data);
    m_dirtyRanges = std::move(_other.m_dirtyRanges);
    m_shouldResize = _other.m_shouldResize;
    m_width = _other.m_width;
    m_height = _other.m_height;
    m_target = _other.m_target;
    m_generateMipmaps = _other.m_generateMipmaps;
    m_disposer = std::move(_other.m_disposer);

    return *this;
}

void Texture::setData(const GLuint* _data, unsigned int _dataSize) {

    m_data.clear();

    m_data.insert(m_data.begin(), _data, _data + _dataSize);

    setDirty(0, m_height);
}

void Texture::setSubData(const GLuint* _subData, uint16_t _xoff, uint16_t _yoff,
                         uint16_t _width, uint16_t _height, uint16_t _stride) {

    size_t bpp = bytesPerPixel();
    size_t divisor = sizeof(GLuint) / bpp;

    // Init m_data if update() was not called after resize()
    if (m_data.size() != (m_width * m_height) / divisor) {
        m_data.resize((m_width * m_height) / divisor);
    }

    // update m_data with subdata
    for (size_t row = 0; row < _height; row++) {

        size_t pos = ((_yoff + row) * m_width + _xoff) / divisor;
        size_t posIn = (row * _stride) / divisor;
        std::memcpy(&m_data[pos], &_subData[posIn], _width * bpp);
    }

    setDirty(_yoff, _height);
}

void Texture::setDirty(size_t _yoff, size_t _height) {
    // FIXME: check that dirty range is valid for texture size!
    size_t max = _yoff + _height;
    size_t min = _yoff;

    if (m_dirtyRanges.empty()) {
        m_dirtyRanges.push_back({min, max});
        return;
    }

    auto n = m_dirtyRanges.begin();

    // Find first overlap
    while (n != m_dirtyRanges.end()) {
        if (min > n->max) {
            // this range is after current
            ++n;
            continue;
        }
        if (max < n->min) {
            // this range is before current
            m_dirtyRanges.insert(n, {min, max});
            return;
        }
        // Combine with overlapping range
        n->min = std::min(n->min, min);
        n->max = std::max(n->max, max);
        break;
    }
    if (n == m_dirtyRanges.end()) {
        m_dirtyRanges.push_back({min, max});
        return;
    }

    // Merge up to last overlap
    auto it = n+1;
    while (it != m_dirtyRanges.end() && max >= it->min) {
        n->max = std::max(it->max, max);
        it = m_dirtyRanges.erase(it);
    }
}

void Texture::bind(RenderState& rs, GLuint _unit) {
    rs.textureUnit(_unit);
    rs.texture(m_target, m_glHandle);
}

void Texture::generate(RenderState& rs, GLuint _textureUnit) {
    GL::genTextures(1, &m_glHandle);

    bind(rs, _textureUnit);

    GL::texParameteri(m_target, GL_TEXTURE_MIN_FILTER, m_options.filtering.min);
    GL::texParameteri(m_target, GL_TEXTURE_MAG_FILTER, m_options.filtering.mag);

    GL::texParameteri(m_target, GL_TEXTURE_WRAP_S, m_options.wrapping.wraps);
    GL::texParameteri(m_target, GL_TEXTURE_WRAP_T, m_options.wrapping.wrapt);

    m_disposer = Disposer(rs);
}

bool Texture::isValid() const {
    return m_glHandle != 0;
}

void Texture::update(RenderState& rs, GLuint _textureUnit) {

    if (!m_shouldResize && m_dirtyRanges.empty()) {
        return;
    }

    if (m_glHandle == 0) {
        if (m_data.size() == 0) {
            size_t divisor = sizeof(GLuint) / bytesPerPixel();
            m_data.resize((m_width * m_height) / divisor, 0);
        }
    }

    GLuint* data = m_data.size() > 0 ? m_data.data() : nullptr;

    update(rs, _textureUnit, data);

    m_data.clear();
}

void Texture::update(RenderState& rs, GLuint _textureUnit, const GLuint* data) {

    if (!m_shouldResize && m_dirtyRanges.empty()) {
        return;
    }

    if (m_glHandle == 0) {
        // texture hasn't been initialized yet, generate it
        generate(rs, _textureUnit);
    } else {
        bind(rs, _textureUnit);
    }

    // resize or push data
    if (m_shouldResize) {
        if (Hardware::maxTextureSize < m_width || Hardware::maxTextureSize < m_height) {
            LOGW("The hardware maximum texture size is currently reached");
        }

        GL::texImage2D(m_target, 0, m_options.internalFormat,
                       m_width, m_height, 0, m_options.format,
                       GL_UNSIGNED_BYTE, data);

        if (data && m_generateMipmaps) {
            // generate the mipmaps for this texture
            GL::generateMipmap(m_target);
        }
        m_shouldResize = false;
        m_dirtyRanges.clear();
        return;
    }
    size_t bpp = bytesPerPixel();
    size_t divisor = sizeof(GLuint) / bpp;

    for (auto& range : m_dirtyRanges) {
        size_t offset =  (range.min * m_width) / divisor;
        GL::texSubImage2D(m_target, 0, 0, range.min, m_width, range.max - range.min,
                          m_options.format, GL_UNSIGNED_BYTE,
                          data + offset);
    }
    m_dirtyRanges.clear();
}

void Texture::resize(const unsigned int _width, const unsigned int _height) {
    m_width = _width;
    m_height = _height;

    if (!(Hardware::supportsTextureNPOT) &&
        !(isPowerOfTwo(m_width) && isPowerOfTwo(m_height)) &&
        (m_generateMipmaps || isRepeatWrapping(m_options.wrapping))) {
        LOGW("OpenGL ES doesn't support texture repeat wrapping for NPOT textures nor mipmap textures");
        LOGW("Falling back to LINEAR Filtering");
        m_options.filtering = {GL_LINEAR, GL_LINEAR};
        m_generateMipmaps = false;
    }

    m_shouldResize = true;
    m_dirtyRanges.clear();
}

bool Texture::isRepeatWrapping(TextureWrapping _wrapping) {
    return _wrapping.wraps == GL_REPEAT || _wrapping.wrapt == GL_REPEAT;
}

size_t Texture::bytesPerPixel() {
    switch (m_options.internalFormat) {
        case GL_ALPHA:
        case GL_LUMINANCE:
            return 1;
        case GL_LUMINANCE_ALPHA:
            return 2;
        case GL_RGB:
            return 3;
        default:
            return 4;
    }
}

void Texture::flipImageData(unsigned char *result, int w, int h, int depth) {

    assert(depth > 0 && w > 0 && h > 0 && bool(result));

    const int step = 512;
    unsigned char temp[step];

    const int stride = w * depth;
    const int end = stride % step;

    for (int row = 0; row < h/2; row++) {
        unsigned char* upper = result + row * stride;
        unsigned char* lower = result + (h - row - 1) * stride;

        for (int col = 0; col + step <= stride; col += step) {
            std::copy(upper, upper + step, temp);
            std::copy(lower, lower + step, upper);
            std::copy(temp, temp + step, lower);
            upper += step;
            lower += step;
        }
        if (end != 0) {
            std::copy(upper, upper + end, temp);
            std::copy(lower, lower + end, upper);
            std::copy(temp, temp + end, lower);
        }
    }
}

void Texture::flipImageData(GLuint *result, int w, int h) {

    assert(w > 0 && h > 0 && bool(result));

    const int step = 512;
    GLuint temp[step];

    const int stride = w;
    const int end = stride % step;

    for (int row = 0; row < h/2; row++) {
        GLuint* upper = result + row * stride;
        GLuint* lower = result + (h - row - 1) * stride;

        for (int col = 0; col + step <= stride; col += step) {
            std::copy(upper, upper + step, temp);
            std::copy(lower, lower + step, upper);
            std::copy(temp, temp + step, lower);
            upper += step;
            lower += step;
        }
        if (end != 0) {
            std::copy(upper, upper + end, temp);
            std::copy(lower, lower + end, upper);
            std::copy(temp, temp + end, lower);
        }
    }
}

}
