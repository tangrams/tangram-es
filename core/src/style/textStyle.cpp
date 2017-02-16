#include "style/textStyle.h"
#include "style/textStyleBuilder.h"

#include "gl/dynamicQuadMesh.h"
#include "gl/mesh.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "labels/textLabels.h"
#include "log.h"
#include "text/fontContext.h"
#include "view/view.h"

#include "text_fs.h"
#include "sdf_fs.h"
#include "point_vs.h"

namespace Tangram {

TextStyle::TextStyle(std::string _name, std::shared_ptr<FontContext> _fontContext,
                     bool _sdf, Blending _blendMode, GLenum _drawMode, bool _selection)
    : Style(_name, _blendMode, _drawMode, _selection), m_sdf(_sdf),
      m_context(_fontContext) {}

TextStyle::~TextStyle() {}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_SHORT, false, 0},
        {"a_uv", 2, GL_UNSIGNED_SHORT, false, 0},
        {"a_selection_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_stroke", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_alpha", 1, GL_UNSIGNED_SHORT, true, 0},
        {"a_scale", 1, GL_UNSIGNED_SHORT, false, 0},
    }));
}

void TextStyle::constructShaderProgram() {

    if (m_sdf) {
        m_shaderSource->setSourceStrings(SHADER_SOURCE(sdf_fs),
                                         SHADER_SOURCE(point_vs));
    } else {
        m_shaderSource->setSourceStrings(SHADER_SOURCE(text_fs),
                                         SHADER_SOURCE(point_vs));
    }

    m_shaderSource->addSourceBlock("defines", "#define TANGRAM_TEXT\n");
}

void TextStyle::onBeginUpdate() {

    // Clear vertices from previous frame
    for (auto& mesh : m_meshes) { mesh->clear(); }

    // Ensure that meshes are available to push to on labels::update()
    size_t s = m_context->glyphTextureCount();
    while (m_meshes.size() < s) {
        m_meshes.push_back(std::make_unique<DynamicQuadMesh<TextVertex>>(m_vertexLayout, GL_TRIANGLES));
    }
}

void TextStyle::onBeginFrame(RenderState& rs) {

    // Upload meshes and textures
    m_context->updateTextures(rs);

    for (auto& mesh : m_meshes) {
        mesh->upload(rs);
    }
}

void TextStyle::onBeginDrawFrame(RenderState& rs, const View& _view, Scene& _scene) {

    Style::onBeginDrawFrame(rs, _view, _scene);

    auto texUnit = rs.nextAvailableTextureUnit();

    m_shaderProgram->setUniformf(rs, m_mainUniforms.uMaxStrokeWidth,
                                 m_context->maxStrokeWidth());
    m_shaderProgram->setUniformf(rs, m_mainUniforms.uTexScaleFactor,
                                 glm::vec2(1.0f / GlyphTexture::size));
    m_shaderProgram->setUniformi(rs, m_mainUniforms.uTex, texUnit);
    m_shaderProgram->setUniformMatrix4f(rs, m_mainUniforms.uOrtho,
                                        _view.getOrthoViewportMatrix());

    if (m_sdf) {
        m_shaderProgram->setUniformi(rs, m_mainUniforms.uPass, 1);

        for (size_t i = 0; i < m_meshes.size(); i++) {
            if (m_meshes[i]->isReady()) {
                m_context->bindTexture(rs, i, texUnit);
                m_meshes[i]->draw(rs, *m_shaderProgram);
            }
        }
        m_shaderProgram->setUniformi(rs, m_mainUniforms.uPass, 0);
    }

    for (size_t i = 0; i < m_meshes.size(); i++) {
        if (m_meshes[i]->isReady()) {
            m_context->bindTexture(rs, i, texUnit);
            m_meshes[i]->draw(rs, *m_shaderProgram);
        }
    }
}

void TextStyle::onBeginDrawSelectionFrame(RenderState& rs, const View& _view, Scene& _scene) {
    if (!m_selection) { return; }

    for (auto& mesh : m_meshes) { mesh->upload(rs); }

    Style::onBeginDrawSelectionFrame(rs, _view, _scene);

    m_selectionProgram->setUniformMatrix4f(rs, m_selectionUniforms.uOrtho,
                                           _view.getOrthoViewportMatrix());

    for (const auto& mesh : m_meshes) {
        if (mesh->isReady()) {
            mesh->draw(rs, *m_selectionProgram, false);
        }
    }
}

std::unique_ptr<StyleBuilder> TextStyle::createBuilder() const {
    return std::make_unique<TextStyleBuilder>(*this);
}


DynamicQuadMesh<TextVertex>& TextStyle::getMesh(size_t id) const {
    if (id >= m_meshes.size()) {
        LOGE("Accessing inconsistent quad mesh");
        assert(false);
        return *m_meshes[0];
    }

    return *m_meshes[id];
}

size_t TextStyle::dynamicMeshSize() const {
    size_t size = 0;
    for (const auto& mesh : m_meshes) {
        size += mesh->bufferSize();
    }

    return size;
}



}
