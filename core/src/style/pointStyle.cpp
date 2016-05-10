#include "pointStyle.h"

#include "platform.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"
#include "gl/dynamicQuadMesh.h"
#include "gl/vertexLayout.h"
#include "scene/spriteAtlas.h"
#include "style/pointStyleBuilder.h"
#include "view/view.h"

namespace Tangram {

PointStyle::PointStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode) {}

PointStyle::~PointStyle() {}

void PointStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_SHORT, false, 0},
        {"a_uv", 2, GL_UNSIGNED_SHORT, true, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_extrude", 2, GL_SHORT, false, 0},
        {"a_screen_position", 2, GL_SHORT, false, 0},
        {"a_alpha", 1, GL_UNSIGNED_BYTE, true, 0},
        {"a_scale", 1, GL_UNSIGNED_BYTE, false, 0},
        {"a_rotation", 1, GL_SHORT, false, 0},
    }));
}

void PointStyle::constructShaderProgram() {

    std::string fragShaderSrcStr = stringFromFile("shaders/point.fs", PathType::internal);
    std::string vertShaderSrcStr = stringFromFile("shaders/point.vs", PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    std::string defines;

    if (!m_spriteAtlas && !m_texture) {
        defines += "#define TANGRAM_POINT\n";
    }

    m_shaderProgram->addSourceBlock("defines", defines);

    m_mesh = std::make_unique<DynamicQuadMesh<SpriteVertex>>(m_vertexLayout, m_drawMode);
}

void PointStyle::onBeginUpdate() {
    m_mesh->clear();
}

void PointStyle::onBeginFrame() {
    // Upload meshes for next frame
    m_mesh->upload();
}

void PointStyle::onBeginDrawFrame(const View& _view, Scene& _scene) {
    Style::onBeginDrawFrame(_view, _scene);

    auto texUnit = RenderState::nextAvailableTextureUnit();

    if (m_spriteAtlas) {
        m_spriteAtlas->bind(texUnit);
    } else if (m_texture) {
        m_texture->update(texUnit);
        m_texture->bind(texUnit);
    }

    m_shaderProgram->setUniformi(m_uTex, texUnit);
    m_shaderProgram->setUniformMatrix4f(m_uOrtho, _view.getOrthoViewportMatrix());

    m_mesh->draw(*m_shaderProgram);
}
std::unique_ptr<StyleBuilder> PointStyle::createBuilder() const {
    return std::make_unique<PointStyleBuilder>(*this);
}

}
