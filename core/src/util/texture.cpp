#include "texture.h"

Texture::Texture(unsigned int _width, unsigned int _height, GLuint _slot, TextureOptions _options) : m_options(_options), m_slot(_slot) {

    m_name = 0;
    m_dirty = false;
    m_shouldResize = false;
    
    resize(_width, _height);
}

Texture::~Texture() {
    glDeleteTextures(1, &m_name);
}

void Texture::bind() {

    glActiveTexture(getTextureUnit());

    glBindTexture(GL_TEXTURE_2D, m_name);
}

void Texture::unbind() {

    glActiveTexture(getTextureUnit());

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::setData(const GLuint* _data, unsigned int _dataSize) {

    if (m_data.size() > 0) {
        m_data.clear();
    }

    m_data.insert(m_data.begin(), _data, _data + _dataSize);

    m_dirty = true;
}

void Texture::setSubData(const GLuint* _subData, unsigned int _xoff, unsigned int _yoff, unsigned int _width, unsigned int _height) {

    std::unique_ptr<std::vector<GLuint>> subData(new std::vector<GLuint>);

    subData->insert(subData->begin(), _subData, _subData + (_width * _height));

    m_subData.push(std::unique_ptr<TextureSubData>(new TextureSubData
        {std::move(subData), _xoff, _yoff, _width, _height}
    ));

    m_dirty = true;
}

void Texture::update() {

    if (!m_dirty) {
        return;
    }

    if (m_name == 0) {
        glGenTextures(1, &m_name);

        bind();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_options.m_filtering.m_min);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_options.m_filtering.m_mag);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_options.m_wrapping.m_wraps);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_options.m_wrapping.m_wrapt);
    } else {

        bind();
    }

    GLuint* data;

    if (m_shouldResize) {
        data = nullptr;
        m_shouldResize = false;
    } else {
        data = m_data.data();
    }

    glTexImage2D(GL_TEXTURE_2D, 0, m_options.m_internalFormat, m_width, m_height, 0, m_options.m_format, GL_UNSIGNED_BYTE, data);

    while (m_subData.size() > 0) {
        const TextureSubData* subData = m_subData.front().get();
            
        glTexSubImage2D(GL_TEXTURE_2D, 0, subData->m_xoff, subData->m_yoff, subData->m_width, subData->m_height,
                        m_options.m_format, GL_UNSIGNED_BYTE, subData->m_data->data());

        m_subData.pop();
    }

    unbind();

    m_dirty = false;
}

void Texture::resize(const unsigned int _width, const unsigned int _height) {

    // don't treat those textures for now
    if (!isPowerOf2(_width) || !isPowerOf2(_height)) {
        logMsg("[Texture] non-power of two textures not implemented yet");
        return;
    }

    m_width = _width;
    m_height = _height;

    m_shouldResize = true;
    m_dirty = true;
}

GLuint Texture::getTextureUnit() {
    switch(m_slot) {
        case 1: return GL_TEXTURE1;
        case 2: return GL_TEXTURE2;
        case 3: return GL_TEXTURE3;
        case 4: return GL_TEXTURE4;
        case 5: return GL_TEXTURE5;
        default: return GL_TEXTURE0;
    }
}
