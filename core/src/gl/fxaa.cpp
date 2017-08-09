#include "fxaa.h"

#include "fxaa_vs.h"
#include "fxaa_fs.h"

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace Tangram {

Fxaa::Fxaa() {

    m_shader = std::make_unique<ShaderProgram>();

    m_shader->setShaderSource(SHADER_SOURCE(fxaa_vs),
                              SHADER_SOURCE(fxaa_fs));

    m_vertexLayout = std::unique_ptr<VertexLayout>(new VertexLayout({
                {"a_position", 2, GL_FLOAT, false, 0},
                {"a_uv", 2, GL_FLOAT, false, 0},}));

}

Fxaa::~Fxaa() {}

void Fxaa::draw(RenderState& _rs, Texture& _tex, glm::vec2 _dim) {

    if (!m_shader->use(_rs)) { return; }

    _rs.vertexBuffer(0);
    _rs.depthTest(GL_FALSE);

    float w = _tex.getWidth();
    float h = _tex.getHeight();

    if (_dim != glm::vec2(0)) {
        w = _dim.x;
        h = _dim.y;
    }

    glm::mat4 proj = glm::ortho(0.f, w, h, 0.f, -1.f, 1.f);
    m_shader->setUniformf(_rs, m_uResolution, w, h);
    m_shader->setUniformMatrix4f(_rs, m_uProj, proj);

    glm::vec2 _pos(0);

    glm::vec4 vertices[6] = {
        {_pos.x, _pos.y, 0, 1},
        {_pos.x, _pos.y + h, 0, 0},
        {_pos.x + w, _pos.y, 1, 1},

        {_pos.x + w, _pos.y, 1, 1},
        {_pos.x, _pos.y + h, 0, 0},
        {_pos.x + w, _pos.y + h, 1, 0},
    };

    _tex.bind(_rs, 0);

    // enable the layout for the _polygon vertices
    m_vertexLayout->enable(_rs, *m_shader, 0, (void*)vertices);

    GL::drawArrays(GL_TRIANGLES, 0, 6);
}

}
