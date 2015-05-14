#include <cstdio>
#include "debugTextStyle.h"

DebugTextStyle::DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color, bool _sdf, GLenum _drawMode)
: TextStyle(_fontName, _name, _fontSize, _color, _sdf, false, _drawMode) {

}

void DebugTextStyle::addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) const {

    if (Tangram::getDebugFlag(Tangram::DebugFlags::TILE_INFOS)) {
        prepareDataProcessing(_tile);

        Mesh* mesh = new Mesh(m_vertexLayout, m_drawMode);

        auto labelContainer = LabelContainer::GetInstance();
        auto ftContext = labelContainer->getFontContext();
        auto textBuffer = _tile.getTextBuffer(*this);

        ftContext->setFont(m_fontName, m_fontSize * m_pixelScale);

        if (m_sdf) {
            float blurSpread = 2.5;
            ftContext->setSignedDistanceField(blurSpread);
        }

        std::string tileID = std::to_string(_tile.getID().x) + "/" + std::to_string(_tile.getID().y) + "/" + std::to_string(_tile.getID().z);
        labelContainer->addLabel(_tile.getID(), m_name, { glm::vec2(0), glm::vec2(0) }, tileID, Label::Type::DEBUG, processedTile->getModelMatrix());

        std::vector<PosTexID> vertices;
        vertices.resize(textBuffer->getVerticesSize());

        if (textBuffer->getVertices(reinterpret_cast<float*>(vertices.data()))) {
            mesh->addVertices(std::move(vertices), {});
        }

        mesh->compileVertexBuffer();

        _tile.addGeometry(*this, std::unique_ptr<VboMesh>(mesh));

        finishDataProcessing(_tile);
    }

}
