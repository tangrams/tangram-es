#include "texture.h"

#include "platform.h"
#include "util/geom.h"
#include "gl/renderState.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstring> // for memset

namespace Tangram {

int Texture::s_validGeneration = 0;

Texture::Texture(unsigned int _width, unsigned int _height, TextureOptions _options, bool _generateMipmaps)
    : m_options(_options), m_generateMipmaps(_generateMipmaps) {

    m_glHandle = 0;
    m_shouldResize = false;
    m_target = GL_TEXTURE_2D;
    m_generation = -1;

    resize(_width, _height);
}

Texture::Texture(const std::string& _file, TextureOptions _options, bool _generateMipmaps)
    : Texture(0, 0, _options, _generateMipmaps) {

    unsigned int size;
    unsigned char* data = bytesFromFile(_file.c_str(), PathType::resource, &size);
    unsigned char* pixels;
    int width, height, comp;

    if (data == nullptr || size == 0) {
        LOGE("Texture not found! '%s'", _file.c_str());
        return;
    }

    pixels = stbi_load_from_memory(data, size, &width, &height, &comp, STBI_rgb_alpha);

    resize(width, height);
    setData(reinterpret_cast<GLuint*>(pixels), width * height);

    free(data);
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

    glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, m_options.m_filtering.m_min);
    glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, m_options.m_filtering.m_mag);

    glTexParameteri(m_target, GL_TEXTURE_WRAP_S, m_options.m_wrapping.m_wraps);
    glTexParameteri(m_target, GL_TEXTURE_WRAP_T, m_options.m_wrapping.m_wrapt);

    m_generation = s_validGeneration;
}

void Texture::checkValidity() {

    if (m_generation != s_validGeneration) {
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
        glTexImage2D(m_target, 0, m_options.m_internalFormat,
                     m_width, m_height, 0, m_options.m_format,
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
                        m_options.m_format, GL_UNSIGNED_BYTE,
                        data + offset);
    }
    m_dirtyRanges.clear();
}

void Texture::resize(const unsigned int _width, const unsigned int _height) {
    m_width = _width;
    m_height = _height;

    m_shouldResize = true;
    m_dirtyRanges.clear();
}

size_t Texture::bytesPerPixel() {
    switch (m_options.m_internalFormat) {
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

void Texture::invalidateAllTextures() {

    ++s_validGeneration;

}

}
