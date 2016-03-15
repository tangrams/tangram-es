#include "texture.h"

#include "platform.h"
#include "util/geom.h"
#include "gl/renderState.h"
#include "gl/hardware.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstring> // for memset

namespace Tangram {

Texture::Texture(unsigned int _width, unsigned int _height, TextureOptions _options, bool _generateMipmaps)
    : m_options(_options), m_generateMipmaps(_generateMipmaps) {

    m_glHandle = 0;
    m_shouldResize = false;
    m_target = GL_TEXTURE_2D;
    m_generation = -1;

    resize(_width, _height);
}

Texture::Texture(const std::string& _file, TextureOptions _options, bool _generateMipmaps)
    : Texture(0u, 0u, _options, _generateMipmaps) {

    unsigned int size;
    unsigned char* data;

    data = bytesFromFile(_file.c_str(), PathType::resource, &size);

    loadPNG(data, size);

    free(data);
}

Texture::Texture(const unsigned char* data, size_t dataSize, TextureOptions options, bool generateMipmaps)
    : Texture(0u, 0u, options, generateMipmaps) {

    loadPNG(data, dataSize);
}

void Texture::loadPNG(const unsigned char* blob, unsigned int size) {
    if (blob == nullptr || size == 0) {
        LOGE("Texture data is empty!");
        return;
    }

    unsigned char* pixels;
    int width, height, comp;

    pixels = stbi_load_from_memory(blob, size, &width, &height, &comp, STBI_rgb_alpha);

    resize(width, height);
    setData(reinterpret_cast<GLuint*>(pixels), width * height);

    stbi_image_free(pixels);
}

Texture::~Texture() {
    if (m_glHandle) {
        glDeleteTextures(1, &m_glHandle);

        // if the texture is bound, and deleted, the binding defaults to 0
        // according to the OpenGL spec, in this case we need to force the
        // currently bound texture to 0 in the render states
        if (RenderState::texture.compare(m_target, m_glHandle)) {
            RenderState::texture.init(m_target, 0, false);
        }
    }
}

void Texture::setData(const GLuint* _data, unsigned int _dataSize) {

    if (m_data.size() > 0) { m_data.clear(); }

    m_data.insert(m_data.begin(), _data, _data + _dataSize);

    setDirty(0, m_height);
}

void Texture::setSubData(const GLuint* _subData, uint16_t _xoff, uint16_t _yoff,
                         uint16_t _width, uint16_t _height) {

    size_t bpp = bytesPerPixel();
    size_t divisor = sizeof(GLuint) / bpp;

    // Init m_data if update() was not called after resize()
    if (m_data.size() != (m_width * m_height) / divisor) {
        m_data.resize((m_width * m_height) / divisor);
    }

    // update m_data with subdata
    for (size_t row = _yoff, end = row + _height; row < end; row++) {

        size_t pos = (row * m_width + _xoff) / divisor;
        std::memcpy(&m_data[pos], &_subData[pos], _width * bpp);
    }

    setDirty(_yoff, _height);
}

void Texture::setDirty(size_t _yoff, size_t _height) {
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

void Texture::bind(GLuint _unit) {
    RenderState::textureUnit(_unit);
    RenderState::texture(m_target, m_glHandle);
}

void Texture::generate(GLuint _textureUnit) {
    glGenTextures(1, &m_glHandle);

    bind(_textureUnit);

    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, m_options.filtering.min);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, m_options.filtering.mag);

    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, m_options.wrapping.wraps);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, m_options.wrapping.wrapt);

    m_generation = RenderState::generation();
}

void Texture::checkValidity() {

    if (!RenderState::isValidGeneration(m_generation)) {
        m_shouldResize = true;
        m_glHandle = 0;
    }
}

void Texture::update(GLuint _textureUnit) {

    checkValidity();

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

    update(_textureUnit, data);
}

void Texture::update(GLuint _textureUnit, const GLuint* data) {

    checkValidity();

    if (!m_shouldResize && m_dirtyRanges.empty()) {
        return;
    }

    if (m_glHandle == 0) {
        // texture hasn't been initialized yet, generate it
        generate(_textureUnit);
    } else {
        bind(_textureUnit);
    }

    // resize or push data
    if (m_shouldResize) {
        if (Hardware::maxTextureSize < m_width || Hardware::maxTextureSize < m_height) {
            LOGW("The hardware maximum texture size is currently reached");
        }

        glTexImage2D(m_target, 0, m_options.internalFormat,
                     m_width, m_height, 0, m_options.format,
                     GL_UNSIGNED_BYTE, data);

        if (data && m_generateMipmaps) {
            // generate the mipmaps for this texture
            glGenerateMipmap(m_target);
        }
        m_shouldResize = false;
        m_dirtyRanges.clear();
        return;
    }
    size_t bpp = bytesPerPixel();
    size_t divisor = sizeof(GLuint) / bpp;

    for (auto& range : m_dirtyRanges) {
        size_t offset =  (range.min * m_width) / divisor;
        glTexSubImage2D(m_target, 0, 0, range.min, m_width, range.max - range.min,
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

}
