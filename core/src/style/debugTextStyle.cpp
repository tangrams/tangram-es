#include "debugTextStyle.h"

#include "text/fontContext.h"
#include "tile/tile.h"
#include "gl/vboMesh.h"

namespace Tangram {

DebugTextStyle::DebugTextStyle(std::shared_ptr<FontContext> _fontContext, FontID _fontId,
                               std::string _name, float _fontSize, bool _sdf,
                               bool _sdfMultisampling, Blending _blendMode, GLenum _drawMode)
    : TextStyle(_name, _fontContext, _sdf, _sdfMultisampling, _blendMode, _drawMode),
      m_font(_fontId), m_fontSize(_fontSize) {
}

void DebugTextStyle::onBeginBuildTile(Tangram::Tile &_tile) const {

    Parameters params;
    params.fontId = m_font;
    params.fontSize = m_fontSize * m_pixelScale;
    params.blurSpread = m_sdf ? 2.5f : 0.0f;

    TextStyle::onBeginBuildTile(_tile);

    if (Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {

        auto& mesh = _tile.getMesh(*this);
        if (!mesh) {
            mesh.reset(newMesh());
        }

        auto& buffer = static_cast<TextBuffer&>(*mesh);

        Label::Options options;
        options.color = 0xdc3522ff;

        buffer.addLabel(_tile.getID().toString(), { glm::vec2(0) },
                        Label::Type::debug, params, options,
                        *m_fontContext);

        onEndBuildTile(_tile);

    }

}

}
