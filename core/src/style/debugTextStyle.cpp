#include "debugTextStyle.h"

#include "gl/shaderProgram.h"
#include "labels/textLabel.h"
#include "style/material.h"
#include "text/fontContext.h"
#include "tile/tile.h"
#include "tangram.h"
#include "text/textBuffer.h"

namespace Tangram {

DebugTextStyle::DebugTextStyle(std::shared_ptr<FontContext> _fontContext, FontID _fontId,
                               std::string _name, float _fontSize, bool _sdf,
                               bool _sdfMultisampling, Blending _blendMode, GLenum _drawMode)
    : TextStyle(_name, _fontContext, _sdf, _sdfMultisampling, _blendMode, _drawMode),
      m_font(_fontId), m_fontSize(_fontSize) {
}

struct DebugTextStyleBuilder : public StyleBuilder {

    const DebugTextStyle& m_style;

    TextStyle::Parameters m_params;

    TextBuffer::Builder m_builder;

    void setup(const Tile& _tile) override {
        if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
            return;
        }

        m_params.text = _tile.getID().toString();
        m_builder.setup(m_style.vertexLayout());
    }

    std::unique_ptr<StyledMesh> build() override {
        if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
            return nullptr;
        }

        if(!m_builder.prepareLabel(m_style.fontContext(), m_params)) {
            return nullptr;
        }

        m_builder.addLabel(m_params, Label::Type::debug, { glm::vec2(.5f) });

        return m_builder.build();
    }

    const Style& style() const override { return m_style; }

    DebugTextStyleBuilder(const DebugTextStyle& _style, TextStyle::Parameters _params)
        : StyleBuilder(_style), m_style(_style), m_params(_params) {}
};

std::unique_ptr<StyleBuilder> DebugTextStyle::createBuilder() const {
    TextStyle::Parameters params;
    params.fontId = m_font;
    params.fontSize = m_fontSize * m_pixelScale;
    params.blurSpread = m_sdf ? 2.5f : 0.0f;
    params.fill = 0xdc3522ff;

    return std::make_unique<DebugTextStyleBuilder>(*this, params);
}

}
