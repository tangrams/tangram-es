#pragma once

#include "gl.h"
#include "platform.h"
#include "geom.h"
#include <vector>
#include <queue>

struct TextureFiltering {
    GLenum m_min;
    GLenum m_mag;
};

struct TextureWrapping {
    GLenum m_wraps;
    GLenum m_wrapt;
};

struct TextureOptions {
    TextureFiltering m_filtering;
    TextureWrapping m_wrapping;
};

class Texture {

public:

    Texture(unsigned int _width, unsigned int _height, GLuint _slot = 0,
            TextureOptions _options = {{GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER}});
    
    ~Texture();

    void bind();
    void unbind();
    void update();

    void resize(const unsigned int _width, const unsigned int _height);

    unsigned int getWidth() const { return m_width; }
    unsigned int getHeight() const { return m_height; }

    GLuint getTextureSlot() const { return m_slot; }

    void setData(const GLuint* _data, unsigned int _dataSize);
    void setSubData(const GLuint* _subData, unsigned int _xoff, unsigned int _yoff, unsigned int _width, unsigned int _height);

protected:

    struct TextureSubData {
        std::unique_ptr<std::vector<GLuint>> m_data;
        unsigned int m_xoff;
        unsigned int m_yoff;
        unsigned int m_width;
        unsigned int m_height;
    };

    GLuint getTextureUnit();

    TextureOptions m_options;
    std::vector<GLuint> m_data;
    std::queue<std::unique_ptr<TextureSubData>> m_subData;

    GLuint m_name;
    GLuint m_slot;

    bool m_dirty;
    bool m_shouldResize;

    unsigned int m_width;
    unsigned int m_height;

};