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
    TextDisplay(std::string& _fontName, glm::vec2 _textDisplayResolution = glm::vec2(100.0));
    ~TextDisplay();

    void clear();

    void init();

    void draw(glm::mat4 _orthoProj);

private:
    struct TextVertex {
        glm::vec2 uv;
        glm::vec2 pos;
    };

    std::string m_font;
    std::unique_ptr<Texture> m_atlas;
    std::unique_ptr<ShaderProgram> m_shader;
    std::unique_ptr<TypedMesh<TextVertex>> m_mesh;
};

}
