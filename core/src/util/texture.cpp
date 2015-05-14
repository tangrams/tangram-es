#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint Texture::s_boundTextures[] = { 0 };
GLuint Texture::s_activeSlot = GL_TEXTURE0;

Texture::Texture(unsigned int _width, unsigned int _height, bool _autoDelete, TextureOptions _options)
: m_options(_options), m_autoDelete(_autoDelete) {

    m_glHandle = 0;
    m_dirty = false;
    m_shouldResize = false;

    resize(_width, _height);
}

Texture::Texture(const std::string& _file, TextureOptions _options)
: Texture(0, 0, true, _options) {

    unsigned int size;
    unsigned char* data = bytesFromResource(_file.c_str(), &size);
    unsigned char* pixels;
    int width, height, comp;

    pixels = stbi_load_from_memory(data,size, &width, &height, &comp, STBI_rgb_alpha);

    resize(width, height);
    setData(reinterpret_cast<GLuint*>(pixels), width * height);
    update(0);

    free(data);
    stbi_image_free(pixels);
}

Texture::~Texture() {
    if (m_autoDelete) {
        destroy();
    }
}

void Texture::destroy() {
    
    for (size_t i = 0; i < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS; i++) {
        if (s_boundTextures[i] == m_glHandle) {
            s_boundTextures[i] = 0;
        }
    }
    glDeleteTextures(1, &m_glHandle);
    
}

void Texture::bind(GLuint _textureSlot) {
    
    if (_textureSlot >= GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) {
        
        // Trying to access an invalid texture unit
        return;
    }
    
    if (_textureSlot != s_activeSlot) {
        
        glActiveTexture(getTextureUnit(_textureSlot));
        s_activeSlot = _textureSlot;
        
    }
    
    if (s_boundTextures[_textureSlot] != m_glHandle) {
        
        glBindTexture(GL_TEXTURE_2D, m_glHandle);
        s_boundTextures[_textureSlot] = m_glHandle;
        
    }
    
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

void Texture::update(GLuint _textureUnit) {

    if (!m_dirty) {
        return;
    }

    if (m_glHandle == 0) { // textures hasn't been initialized yet, generate it

        glGenTextures(1, &m_glHandle);
    
        bind(_textureUnit);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_options.m_filtering.m_min);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_options.m_filtering.m_mag);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_options.m_wrapping.m_wraps);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_options.m_wrapping.m_wrapt);
        
        // if no data make sure texture is 0-filled at creation (useful for transform lookup)
        if (m_data.size() == 0) {
            m_data.resize(m_width * m_height);
            memset(m_data.data(), 0, m_data.size());
        }
    } else {
        
        bind(_textureUnit);
    }

    GLuint* data = m_data.size() > 0 ? m_data.data() : nullptr;

    // resize or push data
    if (data || m_shouldResize) {
        glTexImage2D(GL_TEXTURE_2D, 0, m_options.m_internalFormat, m_width, m_height, 0, m_options.m_format, GL_UNSIGNED_BYTE, data);
        m_shouldResize = false;
    }

    // clear cpu data
    if (data) {
        m_data.clear();
    }

    // process queued sub data updates
    while (m_subData.size() > 0) {
        const TextureSubData* subData = m_subData.front().get();

        glTexSubImage2D(GL_TEXTURE_2D, 0, subData->m_xoff, subData->m_yoff, subData->m_width, subData->m_height,
                        m_options.m_format, GL_UNSIGNED_BYTE, subData->m_data->data());

        m_subData.pop();
    }

    m_dirty = false;
}

void Texture::resize(const unsigned int _width, const unsigned int _height) {
    m_width = _width;
    m_height = _height;

    m_shouldResize = true;
    m_dirty = true;
}

GLuint Texture::getTextureUnit(GLuint _unit) {
    if (_unit >= GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) {
        logMsg("Warning: trying to access unavailable texture unit");
    }
    
    return GL_TEXTURE0 + _unit;
}
