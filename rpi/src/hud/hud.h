#pragma once

#include "util/shaderProgram.h"
#include "util/typedMesh.h"

#include "glm/glm.hpp"

struct LineVertex {
    GLfloat x;
    GLfloat y;
    GLfloat z;
};
typedef TypedMesh<LineVertex> LineMesh;

class Hud {
public:

    Hud();
    virtual ~Hud();

    void init();
    void draw();
private:

    std::shared_ptr<ShaderProgram>  m_cursorShader;
    std::shared_ptr<LineMesh>       m_cursorMesh;
};