#include "texture.h"
#include "lodepng.h"
#include <vector>


class TextureCube : public Texture {
    
public:
    TextureCube(std::string _file, GLuint _slot = 0, TextureOptions _options =
                {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}});
    
    void bind() override;
    
    void unbind() override;
    
    void update() override;
    
    void resize(const unsigned int _width, const unsigned int _height) = delete;

    void setData(const GLuint* _data, unsigned int _dataSize) = delete;
    void setSubData(const GLuint* _subData, unsigned int _xoff, unsigned int _yoff, unsigned int _width, unsigned int _height) = delete;
    
    void destroy() override;
    
private:
    
    struct Face {
        GLenum m_face;
        std::vector<unsigned int> m_data;
        int m_offset;
    };
    
    const GLenum CubeMapFace[6] {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };
    
    std::vector<Face> m_faces;
    
    void load(const std::string& _file);

};