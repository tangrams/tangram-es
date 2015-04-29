#include "textureCube.h"

TextureCube::TextureCube(std::string _file, GLuint _slot, TextureOptions _options) : Texture(0, 0, _slot, _options) {
    
    load(_file);
}

void TextureCube::load(const std::string& _file) {
    unsigned int size;
    unsigned char* data = bytesFromResource(_file.c_str(), &size);
    unsigned int width;
    unsigned int height;

    std::vector<unsigned char> png;
    std::vector<unsigned char> image;
    
    png.insert(png.begin(), data, data + size);
    
    lodepng::decode(image, width, height, png);
    
    size = width * height;
   
    m_width = width / 4;
    m_height = height / 3;
    
    for(int i = 0; i < 6; ++i) {
        GLenum face = CubeMapFace[i];
        m_faces.push_back({ face, {}, 0 });
        m_faces.back().m_data.resize(m_width * m_height);
    }
    
    for(int y = 0; y < height; ++y) {
        int jFace = (y - (y % m_height)) / m_height;
        
        for(int iFace = 0; iFace < 4; ++iFace) {
            Face* face = nullptr;
            
            if(iFace == 2 && jFace == 1) face = &m_faces[0]; // POS_X
            if(iFace == 0 && jFace == 1) face = &m_faces[1]; // NEG_X
            if(iFace == 1 && jFace == 0) face = &m_faces[2]; // POS_Y
            if(iFace == 1 && jFace == 2) face = &m_faces[3]; // NEG_Y
            if(iFace == 1 && jFace == 1) face = &m_faces[4]; // POS_Z
            if(iFace == 3 && jFace == 1) face = &m_faces[5]; // NEG_Z
            
            if (!face) {
                continue;
            }
            
            int offset = (m_width * iFace + y * width) * sizeof(unsigned int);
            memcpy(face->m_data.data() + face->m_offset, image.data() + offset, m_width * sizeof(unsigned int));
            face->m_offset += m_width;
        }
    }
    
    update();
}

void TextureCube::bind() {
    
    glActiveTexture(getTextureUnit());
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_name);
}

void TextureCube::unbind() {
    
    glActiveTexture(getTextureUnit());
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void TextureCube::update() {
    if (m_name != 0 || m_faces.size() == 0) {
        return;
    }
        
    glGenTextures(1, &m_name);
    
    bind();
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, m_options.m_filtering.m_min);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, m_options.m_filtering.m_mag);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, m_options.m_wrapping.m_wraps);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, m_options.m_wrapping.m_wrapt);
    
    for(int i = 0; i < 6; ++i) {
        const Face& f = m_faces[i];
        glTexImage2D(CubeMapFace[i], 0, m_options.m_internalFormat, m_width, m_height, 0, m_options.m_format, GL_UNSIGNED_BYTE, f.m_data.data());
    }
    
    m_data.clear();
    
    unbind();
}
