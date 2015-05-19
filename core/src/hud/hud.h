#pragma once

#include "util/shaderProgram.h"

#include "shapes.h"
#include "vbo.h"

#include "glm/glm.hpp"

class Hud {
public:

    Hud();
    virtual ~Hud();

    void mousePosition(int _x, int _y);
    void setWindowSize(int _width, int _height);

    void init();
    void draw();
private:

    glm::vec2   m_windowSize;
    glm::mat4   m_orthoMatrix;

// Cursor
    glm::vec2       m_mousePos;
    std::shared_ptr<ShaderProgram>   m_cursorShader;
    std::shared_ptr<Vbo>             m_cursorGeom;
};