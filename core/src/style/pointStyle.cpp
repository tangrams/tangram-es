#include "style/pointStyle.h"

#include "gl/dynamicQuadMesh.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"
#include "gl/vertexLayout.h"
#include "platform.h"
#include "scene/spriteAtlas.h"
#include "style/pointStyleBuilder.h"
#include "view/view.h"

#include "point_vs.h"
#include "point_fs.h"

#include "log.h"

namespace Tangram {

PointStyle::PointStyle(std::string _name, Blending _blendMode, GLenum _drawMode, bool _selection)
    : Style(_name, _blendMode, _drawMode, _selection) {

    m_type = StyleType::point;
    m_lightingType = LightingType::none;

    m_textStyle = std::make_unique<TextStyle>(_name, true, _blendMode, _drawMode);
}

PointStyle::~PointStyle() {}

void PointStyle::build(const Scene& _scene) {
    Style::build(_scene);

    m_textStyle->build(_scene);

    m_mesh = std::make_unique<DynamicQuadMesh<SpriteVertex>>(m_vertexLayout, m_drawMode);
}

void PointStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 4, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_SHORT, true, 0},
        {"a_selection_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_outline_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_aa_factor", 1, GL_SHORT, true, 0},
        {"a_alpha", 1, GL_UNSIGNED_SHORT, true, 0},
    }));
}

void PointStyle::constructShaderProgram() {
    m_shaderSource->setSourceStrings(point_fs, point_vs);
}

void PointStyle::onBeginUpdate() {
    m_mesh->clear();
    m_batches.clear();
    m_textStyle->onBeginUpdate();
}

void PointStyle::onBeginFrame(RenderState& rs) {
    // Upload meshes for next frame
    m_mesh->upload(rs);
    m_textStyle->onBeginFrame(rs);
}

void PointStyle::onBeginDrawFrame(RenderState& rs, const View& _view) {
    Style::onBeginDrawFrame(rs, _view);

    auto texUnit = rs.nextAvailableTextureUnit();

    m_shaderProgram->setUniformi(rs, m_mainUniforms.uTex, texUnit);

    m_shaderProgram->setUniformMatrix4f(rs, m_mainUniforms.uOrtho,
                                        _view.getOrthoViewportMatrix());


    size_t vertexPos = 0;
    for (auto& batch : m_batches) {

        auto tex = batch.texture;

        m_shaderProgram->setUniformi(rs, m_mainUniforms.uSpriteMode, bool(tex) ? 1 : 0);

        if (tex) { tex->bind(rs, texUnit); }

        m_mesh->drawRange(rs, *m_shaderProgram, vertexPos, batch.vertexCount);

        vertexPos += batch.vertexCount;
    }

    m_textStyle->onBeginDrawFrame(rs, _view);
}

void PointStyle::onBeginDrawSelectionFrame(RenderState& rs, const View& _view) {
    if (!m_selection) { return; }

    m_mesh->upload(rs);

    Style::onBeginDrawSelectionFrame(rs, _view);

    m_selectionProgram->setUniformMatrix4f(rs, m_selectionUniforms.uOrtho,
                                           _view.getOrthoViewportMatrix());

    m_mesh->draw(rs, *m_selectionProgram, false);

    m_textStyle->onBeginDrawSelectionFrame(rs, _view);
}

std::unique_ptr<StyleBuilder> PointStyle::createBuilder() const {
    return std::make_unique<PointStyleBuilder>(*this);
}

void PointStyle::setPixelScale(float _pixelScale) {
    Style::setPixelScale(_pixelScale);
    m_textStyle->setPixelScale(_pixelScale);
}

SpriteVertex* PointStyle::pushQuad(Texture* texture) const {

    if (m_batches.empty() || m_batches.back().texture != texture) {
        m_batches.push_back({ texture });
    }

    m_batches.back().vertexCount += 4;

    return m_mesh->pushQuad();
}

}
