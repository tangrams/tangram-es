#pragma once

#include "util/shaderProgram.h"

#include "shapes.h"
#include "glm/glm.hpp"
#include "rectangle.h"

class SlideZoom : public Rectangle {
public:

    void init();
    void draw();

    float zoom;
    float slider;

private:
    std::shared_ptr<ShaderProgram>  m_fixShader;
    std::shared_ptr<ShaderProgram>  m_trnShader;

    std::shared_ptr<HudMesh>        m_verticalRulerMeshA;
    std::shared_ptr<HudMesh>        m_verticalRulerMeshB;
    std::shared_ptr<HudMesh>        m_triangle;
    std::shared_ptr<HudMesh>        m_fixed;
};
