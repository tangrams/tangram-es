#include "debugTextStyle.h"

#include "text/fontContext.h"
#include "tile/tile.h"
#include "gl/vboMesh.h"

namespace Tangram {

DebugTextStyle::DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize, bool _sdf, GLenum _drawMode)
: TextStyle(_name, _drawMode, _sdf), m_fontName(_fontName), m_fontSize(_fontSize) {
}

void DebugTextStyle::onBeginBuildTile(Tangram::Tile &_tile) const {

    Parameters params;
    params.fontKey = m_fontName;
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

        buffer.addLabel(_tile.getID().toString(), { glm::vec2(0) }, Label::Type::debug, params, options);

        onEndBuildTile(_tile);

    }

}

}
