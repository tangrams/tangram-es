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

    const DebugTextStyle& m_style;

    TextStyle::Parameters m_params;

    void begin(const Tile& _tile) override {
        if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
            return;
        }

        m_params.text = _tile.getID().toString();
    }

    std::unique_ptr<VboMesh> build() override {
        if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
            return nullptr;
        }

        auto mesh = std::make_unique<TextBuffer>(m_style.vertexLayout());
        mesh->addLabel(m_params, { glm::vec2(.5f) }, Label::Type::debug, m_style.fontContext());

        return std::move(mesh);
    }

    Builder(const DebugTextStyle& _style, TextStyle::Parameters _params)
        : StyleBuilder(_style), m_style(_style), m_params(_params) {}
};
}

std::unique_ptr<StyleBuilder> DebugTextStyle::createBuilder() const {
    TextStyle::Parameters params;
    params.fontId = m_font;
    params.fontSize = m_fontSize * m_pixelScale;
    params.blurSpread = m_sdf ? 2.5f : 0.0f;
    params.fill = 0xdc3522ff;

    return std::make_unique<Builder>(*this, params);
}

}
