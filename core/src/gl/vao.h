#pragma once

#include "gl.h"
#include <vector>
#include <string>

namespace Tangram {

class ShaderProgram;
class VertexLayout;

class Vao {

public:
    Vao();
    ~Vao();

    void init(ShaderProgram& _program, const std::vector<std::pair<uint32_t, uint32_t>>& _vertexOffsets,
              VertexLayout& _layout, GLint _vertexBuffer, GLint _indexBuffer);

    void bind(unsigned int _index);
    void unbind();

private:
    GLuint* m_glVAOs;
    GLuint m_glnVAOs;

};

}

