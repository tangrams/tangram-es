#include "textStyle.h"
#include "textStyleBuilder.h"

#include "gl/shaderProgram.h"
#include "gl/mesh.h"
#include "gl/renderState.h"
#include "gl/dynamicQuadMesh.h"
#include "labels/textLabels.h"
#include "text/fontContext.h"
#include "view/view.h"

#include "shaders/text_fs.h"
#include "shaders/sdf_fs.h"
#include "shaders/point_vs.h"

namespace Tangram {

TextStyle::TextStyle(std::string _name, std::shared_ptr<FontContext> _fontContext,
                     bool _sdf, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode), m_sdf(_sdf),
      m_context(_fontContext ? _fontContext : std::make_shared<FontContext>()) {}

TextStyle::~TextStyle() {}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_SHORT, false, 0},
        {"a_uv", 2, GL_UNSIGNED_SHORT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_stroke", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_screen_position", 2, GL_SHORT, false, 0},
        {"a_alpha", 1, GL_UNSIGNED_BYTE, true, 0},
        {"a_scale", 1, GL_UNSIGNED_BYTE, false, 0},
        {"a_rotation", 1, GL_SHORT, false, 0},
    }));
}

void TextStyle::constructShaderProgram() {

    if (m_sdf) {
        m_shaderProgram->setSourceStrings(SHADER_SOURCE(sdf_fs),
                                          SHADER_SOURCE(point_vs));
    } else {
        m_shaderProgram->setSourceStrings(SHADER_SOURCE(text_fs),
                                          SHADER_SOURCE(point_vs));
    }

    std::string defines = "#define TANGRAM_TEXT\n";

    m_shaderProgram->addSourceBlock("defines", defines);
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

void TextStyle::onBeginFrame() {

    // Upload meshes and textures
    m_context->updateTextures();

    for (auto& mesh : m_meshes) { mesh->upload(); }
}

void TextStyle::onBeginDrawFrame(const View& _view, Scene& _scene) {

    Style::onBeginDrawFrame(_view, _scene);

    auto texUnit = RenderState::nextAvailableTextureUnit();

    m_shaderProgram->setUniformf(m_uMaxStrokeWidth, m_context->maxStrokeWidth());
    m_shaderProgram->setUniformf(m_uTexScaleFactor, glm::vec2(1.0f / GlyphTexture::size));
    m_shaderProgram->setUniformi(m_uTex, texUnit);
    m_shaderProgram->setUniformMatrix4f(m_uOrtho, _view.getOrthoViewportMatrix());

    if (m_sdf) {
        m_shaderProgram->setUniformi(m_uPass, 1);

        for (size_t i = 0; i < m_meshes.size(); i++) {
            if (m_meshes[i]->isReady()) {
                m_context->bindTexture(i, texUnit);
                m_meshes[i]->draw(*m_shaderProgram);
            }
        }
        m_shaderProgram->setUniformi(m_uPass, 0);
    }

    for (size_t i = 0; i < m_meshes.size(); i++) {
        if (m_meshes[i]->isReady()) {
            m_context->bindTexture(i, texUnit);
            m_meshes[i]->draw(*m_shaderProgram);
        }
    }
}

TextStyle::Parameters TextStyle::defaultUnifiedParams() const {
    TextStyle::Parameters params;
    params.anchor = LabelProperty::Anchor::bottom;
    return params;
}

std::unique_ptr<StyleBuilder> TextStyle::createBuilder() const {
    return std::make_unique<TextStyleBuilder>(*this);
}


DynamicQuadMesh<TextVertex>& TextStyle::getMesh(size_t id) const {
    if (id >= m_meshes.size()) {
        LOGE("Accesing inconsistent quad mesh");
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
