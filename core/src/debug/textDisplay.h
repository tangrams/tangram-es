#pragma once

#include <string>
#include <vector>
#include "gl/texture.h"
#include "gl/typedMesh.h"
#include "gl/shaderProgram.h"
#include "glm/vec2.hpp"

namespace Tangram {

typedef int FontID;

class TextDisplay {
public:
    TextDisplay(glm::vec2 _textDisplayResolution = glm::vec2(200.0));
    ~TextDisplay();

    void init();

    void draw(std::vector<std::string> _infos);

private:

    std::unique_ptr<ShaderProgram> m_shader;
    glm::vec2 m_textDisplayResolution;
};

}
