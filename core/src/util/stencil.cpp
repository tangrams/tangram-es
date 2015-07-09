#include "stencil.h"
#include "util/vertexLayout.h"

#include "glm/gtc/type_ptr.hpp"

StencilClipper::StencilClipper()
    : m_vertexLayout(std::shared_ptr<VertexLayout>(new VertexLayout({{"a_position", 2, GL_FLOAT, false, 0 }}))),
      m_mesh(m_vertexLayout, GL_TRIANGLES) {

    auto vertShaderSrc = stringFromResource("stencil.vs");
    auto fragShaderSrc = stringFromResource("stencil.fs");

    m_shader.setSourceStrings(fragShaderSrc, vertShaderSrc);

    //m_mesh.addVertices({{0.99,0.99}, {-0.99,0.99}, {-0.99,-0.99}, {0.99,-0.99}}, {0, 1, 2, 2, 3, 0});
    m_mesh.addVertices({{1,1}, {-1,1}, {-1,-1}, {1,-1}}, {0, 1, 2, 2, 3, 0});
    m_mesh.compileVertexBuffer();
}

void StencilClipper::draw(glm::mat4 modelViewProjMatrix) {
    m_shader.setUniformMatrix4f("u_modelViewProj", glm::value_ptr(modelViewProjMatrix));

    m_mesh.draw(m_shader);
}
