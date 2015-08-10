#include "debugTextStyle.h"

#include "text/fontContext.h"
#include "tile/tile.h"
#include "gl/vboMesh.h"

namespace Tangram {

DebugTextStyle::DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color, bool _sdf, GLenum _drawMode)
: TextStyle(_fontName, _name, _fontSize, _color, _sdf, false, _drawMode) {
}

void DebugTextStyle::onBeginBuildTile(Tangram::Tile &_tile) const {

    TextStyle::onBeginBuildTile(_tile);

    if (Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {

        auto& mesh = _tile.getMesh(*this);
        if (!mesh) {
            mesh.reset(newMesh());
        }

        auto& buffer = static_cast<TextBuffer&>(*mesh);

        auto ftContext = FontContext::GetInstance();
        
        ftContext->setFont(m_fontName, m_fontSize * m_pixelScale);

        if (m_sdf) {
            float blurSpread = 2.5;
            ftContext->setSignedDistanceField(blurSpread);
        }

        std::string tileID = std::to_string(_tile.getID().x) + "/" + std::to_string(_tile.getID().y) + "/" + std::to_string(_tile.getID().z);
        addTextLabel(buffer, { glm::vec2(0), glm::vec2(0) }, tileID, Label::Type::debug);

        onEndBuildTile(_tile);

        mesh->compileVertexBuffer();
    }

}

}
