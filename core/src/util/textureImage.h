#pragma once

#include "texture.h"
#include "lodepng.h"
#include <string>

class TextureImage : public Texture {

public:
    TextureImage(std::string _file, GLuint _slot = 0);

private:
    void load(std::string& _file);
};
