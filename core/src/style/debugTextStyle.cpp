#include "debugTextStyle.h"

#include "gl/shaderProgram.h"
#include "style/material.h"
#include "text/fontContext.h"
#include "text/textBuffer.h"
#include "tile/tile.h"
#include "tangram.h"

namespace Tangram {

DebugTextStyle::DebugTextStyle(std::shared_ptr<FontContext> _fontContext, FontID _fontId,
                               std::string _name, float _fontSize, bool _sdf,
                               bool _sdfMultisampling, Blending _blendMode, GLenum _drawMode)
    : TextStyle(_name, _fontContext, _sdf, _sdfMultisampling, _blendMode, _drawMode),
      m_font(_fontId), m_fontSize(_fontSize) {
}

namespace {
struct Builder : public StyleBuilder {

    Builder(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode,
            std::shared_ptr<FontContext> _fontContext, TextStyle::Parameters _params)
        : StyleBuilder(_vertexLayout, _drawMode),
          m_fontContext(_fontContext), m_params(_params) {}

    std::shared_ptr<FontContext> m_fontContext;
    TextStyle::Parameters m_params;

    void initMesh() override {}

    std::unique_ptr<VboMesh> build() override {
        if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
            return nullptr;
        }

        m_params.text = m_tile->getID().toString();

        auto mesh = std::make_unique<TextBuffer>(m_vertexLayout);
        mesh->addLabel(m_params, { glm::vec2(.5f) }, Label::Type::debug, *m_fontContext);

        return std::move(mesh);
    }
};
}

std::unique_ptr<StyleBuilder> DebugTextStyle::createBuilder() const {
    TextStyle::Parameters params;
    params.fontId = m_font;
    params.fontSize = m_fontSize * m_pixelScale;
    params.blurSpread = m_sdf ? 2.5f : 0.0f;
    params.fill = 0xdc3522ff;

    return std::make_unique<Builder>(m_vertexLayout, m_drawMode,
                                     m_fontContext, params);
}

}
