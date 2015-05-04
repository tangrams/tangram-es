#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "textureImage.h"

TextureImage::TextureImage(std::string _file, GLuint _slot) 
    : Texture (0, 0, _slot, {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}})
{
    load(_file);
}

void TextureImage::load(std::string& _file) {
    unsigned int size;
    unsigned char* data = bytesFromResource(_file.c_str(), &size);
    unsigned char* pixels;
    int width, height, comp;

    pixels = stbi_load_from_memory(data,size, &width, &height, &comp, 0);

    resize(width, height);
    setData(reinterpret_cast<GLuint*>(pixels), width * height);
    update();

    delete [] pixels;
    delete [] data;
}
