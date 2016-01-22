#pragma once

#include <string>
#include "gl/texture.h"
#include "gl/typedMesh.h"
#include "gl/shaderProgram.h"
#include "glm/vec2.hpp"

namespace Tangram {

typedef int FontID;

class TextDisplay {
public:
    TextDisplay(glm::vec2 _textDisplayResolution = glm::vec2(100.0));
    ~TextDisplay();

    void init();

    void draw(glm::mat4 _orthoProj, float _zoom);

private:

    std::unique_ptr<ShaderProgram> m_shader;
    glm::vec2 m_textDisplayResolution;
};

}
