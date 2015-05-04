#pragma once

#include "gl.h"
#include "platform.h"
#include "geom.h"
#include <vector>
#include <queue>
#include <memory>
#include <string>

struct TextureFiltering {
    GLenum m_min;
    GLenum m_mag;
};

struct TextureWrapping {
    GLenum m_wraps;
    GLenum m_wrapt;
};

struct TextureOptions {
    GLenum m_internalFormat;
    GLenum m_format;
    TextureFiltering m_filtering;
    TextureWrapping m_wrapping;
};

class Texture {

public:

    Texture(unsigned int _width, unsigned int _height, GLuint _slot = 0,
            TextureOptions _options = {GL_ALPHA, GL_ALPHA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}});
    
    Texture(const std::string& _file, GLuint _slot = 0, 
            TextureOptions _options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}});

    ~Texture();

    /* Binds the texture to GPU */
    void bind();

    /* Unbinds the texture from GPU */
    void unbind();

    /* Perform texture updates, should be called at least once and after adding data or resizing */
    void update();

    /* Resize the texture 
     * 
     * Width and height should be power of two numbers 
     */
    void resize(const unsigned int _width, const unsigned int _height);

    /* Width and Height texture getters */
    unsigned int getWidth() const { return m_width; }
    unsigned int getHeight() const { return m_height; }

    /* Gets the GPU texture slot */
    GLuint getTextureSlot() const { return m_slot; }

    /* Sets texture data
     * 
     * Has less priority than set sub data
     */ 
    void setData(const GLuint* _data, unsigned int _dataSize);

    /* Update a region of the texture */
    void setSubData(const GLuint* _subData, unsigned int _xoff, unsigned int _yoff, unsigned int _width, unsigned int _height);

    /* GPU delete of the texture */
    void destroy();

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

    // used to queue the subdata updates, each call of setSubData would be treated in the order that they arrived
    std::queue<std::unique_ptr<TextureSubData>> m_subData;

    GLuint m_name;
    GLuint m_slot;

    bool m_dirty;
    bool m_shouldResize;

    unsigned int m_width;
    unsigned int m_height;

};
