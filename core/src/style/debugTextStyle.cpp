#include "debugTextStyle.h"

#include "text/fontContext.h"
#include "tile/tile.h"
#include "gl/vboMesh.h"

namespace Tangram {

DebugTextStyle::DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color, bool _sdf, GLenum _drawMode)
: TextStyle(_fontName, _name, _fontSize, _color, _sdf, false, _drawMode) {
}

void DebugTextStyle::addData(TileData& _data, Tile& _tile) {

    if (Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
        std::shared_ptr<VboMesh> mesh(newMesh());
        auto& buffer = static_cast<TextBuffer&>(*mesh);
        
        onBeginBuildTile(*mesh);
        
        std::string tileID = std::to_string(_tile.getID().x) + "/" + std::to_string(_tile.getID().y) + "/" + std::to_string(_tile.getID().z);
        buffer.addLabel(tileID, { glm::vec2(0), glm::vec2(0) }, Label::Type::debug);

        onEndBuildTile(*mesh);

        mesh->compileVertexBuffer();
        _tile.addMesh(*this, mesh);
    }

}

}
