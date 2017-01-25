#include "skybox.h"

#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "gl/textureCube.h"
#include "view/view.h"

static const char* cubemap_vs = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelViewProj;
attribute vec3 a_position;
varying vec3 v_uv;

const mat3 rotNegHalfPiAroundX = mat3( 1.0,  0.0,  0.0,
        0.0,  0.0, -1.0,
        0.0,  1.0,  0.0);

void main() {
    // The map coordinates use +z as "up" instead of the cubemap convention of +y,
    // so we rotate the texture coordinates by pi/2 around x to correct the orientation
    v_uv = rotNegHalfPiAroundX * vec3(a_position);
    vec4 pos = u_modelViewProj * vec4(a_position, 1.0);

    // force depth to 1.0
    gl_Position = pos.xyww;
}
)";

static const char* cubemap_fs = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform samplerCube u_tex;
varying vec3 v_uv;

void main() {
        gl_FragColor = textureCube(u_tex, v_uv);
}
)";

namespace Tangram {

Skybox::Skybox(std::string _file) : m_file(_file) {}

void Skybox::init() {

    m_shader = std::make_unique<ShaderProgram>();
    m_shader->setSourceStrings(cubemap_fs, cubemap_vs);

    m_texture = std::unique_ptr<Texture>(new TextureCube(m_file));
    auto layout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
    }));

    m_mesh = std::make_unique<Mesh<PosVertex>>(layout, GL_TRIANGLES, GL_STATIC_DRAW);
    m_mesh->compile({
        { 5, 1, 3, 3, 7, 5, // +x
          6, 2, 0, 0, 4, 6, // -x
          2, 6, 7, 7, 3, 2, // +y
          5, 4, 0, 0, 1, 5, // -y
          0, 2, 3, 3, 1, 0, // +z
          7, 6, 4, 4, 5, 7  // -z
        },
        {{ -1.0, -1.0,  1.0 },
         {  1.0, -1.0,  1.0 },
         { -1.0,  1.0,  1.0 },
         {  1.0,  1.0,  1.0 },
         { -1.0, -1.0, -1.0 },
         {  1.0, -1.0, -1.0 },
         { -1.0,  1.0, -1.0 },
         {  1.0,  1.0, -1.0 }}});
}

void Skybox::draw(RenderState& rs, const View& _view) {

    m_texture->update(rs, 0);
    m_texture->bind(rs, 0);

    glm::mat4 vp = _view.getViewProjectionMatrix();

    // Remove translation so that skybox is centered on view
    vp[3] = { 0, 0, 0, 0 };

    m_shader->setUniformMatrix4f(rs, m_uModelViewProj, vp);
    m_shader->setUniformi(rs, m_uTex, 0);

    rs.blending(GL_FALSE);
    rs.depthTest(GL_TRUE);

    m_mesh->draw(rs, *m_shader);

}

}
