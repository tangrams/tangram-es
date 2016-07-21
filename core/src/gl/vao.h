#pragma once

#include "gl.h"
#include <vector>
#include <string>

namespace Tangram {

class RenderState;
class ShaderProgram;
class VertexLayout;

class Vao {

public:
    Vao();
    ~Vao();

    void init(RenderState& rs, ShaderProgram& _program, const std::vector<std::pair<uint32_t, uint32_t>>& _vertexOffsets,
              VertexLayout& _layout, GLuint _vertexBuffer, GLuint _indexBuffer);

    void bind(unsigned int _index);
    void unbind();
    void discard();

private:
    GLuint* m_glVAOs;
    GLuint m_glnVAOs;

};

}

