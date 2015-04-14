#include "textureImage.h"

TextureImage::TextureImage(std::string _file, GLuint _slot) 
    : Texture (0, 0, _slot, {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}})
{
    load(_file);
}

void TextureImage::load(std::string& _file) {
    unsigned int size;
    unsigned char* data = bytesFromResource(_file.c_str(), &size); 
    unsigned int width, height;

    std::vector<unsigned char> png;
    std::vector<unsigned char> image; //the raw pixels

    png.insert(png.begin(), data, data + size);

    lodepng::decode(image, width, height, png);

    resize(width, height);
    setData(reinterpret_cast<GLuint*>(image.data()), width * height);
    update();
}
