#pragma once

#include "gl.h"
#include "platform.h"
#include "geom.h"
#include <vector>
#include <queue>
#include <memory>
#include <unordered_map>
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

    Texture(unsigned int _width, unsigned int _height, bool _autoDelete = true,
            TextureOptions _options = {GL_ALPHA, GL_ALPHA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}});
    
    Texture(const std::string& _file,
            TextureOptions _options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}});

    ~Texture();

    /* Binds the texture to the specified slot */
    void bind(GLuint _textureSlot);

    /* Perform texture updates, should be called at least once and after adding data or resizing */
    void update(GLuint _textureSlot);

    /* Resize the texture */
    void resize(const unsigned int _width, const unsigned int _height);

    /* Width and Height texture getters */
    unsigned int getWidth() const { return m_width; }
    unsigned int getHeight() const { return m_height; }
    
    GLuint getGlHandle() { return m_glHandle; }

    /* Sets texture data
     * 
     * Has less priority than set sub data
     */ 
    void setData(const GLuint* _data, unsigned int _dataSize);

    /* Update a region of the texture */
    void setSubData(const GLuint* _subData, unsigned int _xoff, unsigned int _yoff, unsigned int _width, unsigned int _height);

    /* GPU delete of the texture */
    void destroy();
    
    typedef std::pair<GLuint, GLuint> TextureSlot;
    
protected:

    TextureOptions m_options;
    std::vector<GLuint> m_data;
    GLuint m_glHandle;

    bool m_dirty;
    bool m_shouldResize;

    unsigned int m_width;
    unsigned int m_height;
    
private:
    
    struct TextureSubData {
        std::unique_ptr<std::vector<GLuint>> m_data;
        unsigned int m_xoff;
        unsigned int m_yoff;
        unsigned int m_width;
        unsigned int m_height;
    };
    
    bool m_autoDelete;
    
    // used to queue the subdata updates, each call of setSubData would be treated in the order that they arrived
    std::queue<std::unique_ptr<TextureSubData>> m_subData;
    
    // We refer to both 'texture slots' and 'texture units', which are almost (but not quite) the same.
    // Texture slots range from 0 to GL_MAX_COMBINED_TEXTURE_UNITS-1 and the texture unit corresponding to
    // a given texture slot is (slot + GL_TEXTURE0).
    static GLuint s_activeSlot;
    static GLuint getTextureUnit(GLuint _slot);
    
    // if (s_boundTextures[s] == h) then the texture with handle 'h' is currently bound at slot 's'
    static GLuint s_boundTextures[GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS];

};
