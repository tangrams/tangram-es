#pragma once

#include "util/shaderProgram.h"

#include "shapes.h"
#include "glm/glm.hpp"
#include "rectangle.h"

class SlideRot : public Rectangle {
public:

    void init();
    void draw();

    float angle;

private:
    std::shared_ptr<ShaderProgram>  m_fixShader;
    std::shared_ptr<ShaderProgram>  m_rotShader;
    std::shared_ptr<HudMesh>        m_circularRulerMeshA;
    std::shared_ptr<HudMesh>        m_circularRulerMeshB;
    std::shared_ptr<HudMesh>        m_fixed;
};
