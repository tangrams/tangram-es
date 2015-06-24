#include <cstdio>
#include "debugTextStyle.h"
#include "text/fontContext.h"

DebugTextStyle::DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color,
                               bool _sdf, GLenum _drawMode)
    : TextStyle(_fontName, _name, _fontSize, _color, _sdf, false, _drawMode) {}

void DebugTextStyle::addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) {

    if (Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
        onBeginBuildTile(_tile);

        std::shared_ptr<VboMesh> mesh(new Mesh(m_vertexLayout, m_drawMode));

        auto ftContext = m_labels->getFontContext();
        auto textBuffer = _tile.getTextBuffer(*this);

        ftContext->setFont(m_fontName, m_fontSize * m_pixelScale);

        if (m_sdf) {
            float blurSpread = 2.5;
            ftContext->setSignedDistanceField(blurSpread);
        }

        std::string tileID = std::to_string(_tile.getID().x) + "/" + std::to_string(_tile.getID().y) + "/" +
                             std::to_string(_tile.getID().z);
        m_labels->addLabel(_tile, m_name, {glm::vec2(0), glm::vec2(0)}, tileID, Label::Type::debug);

        onEndBuildTile(_tile, mesh);

        mesh->compileVertexBuffer();
        _tile.addGeometry(*this, mesh);
    }
}
