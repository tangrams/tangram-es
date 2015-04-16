#include <cstdio>
#include "debugTextStyle.h"

DebugTextStyle::DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize, bool _sdf, GLenum _drawMode)
: FontStyle(_fontName, _name, _fontSize, _sdf, _drawMode) {

}

void DebugTextStyle::addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) const {

    if (Tangram::getDebugFlag(Tangram::DebugFlags::TILE_INFOS)) {
        prepareDataProcessing(_tile);

        Mesh* mesh = new Mesh(m_vertexLayout, m_drawMode);

        int nVerts;
        auto labelContainer = LabelContainer::GetInstance();
        auto ftContext = labelContainer->getFontContext();
        auto textBuffer = _tile.getTextBuffer(*this);

        ftContext->setFont(m_fontName, m_fontSize * m_pixelScale);

        if (m_sdf) {
            float blurSpread = 2.5;
            ftContext->setSignedDistanceField(blurSpread);
        }

        std::string tileID = std::to_string(_tile.getID().x) + "/" + std::to_string(_tile.getID().y) + "/" + std::to_string(_tile.getID().z);
        auto label = labelContainer->addLabel(_tile.getID(), m_name, { glm::vec2(0), glm::vec2(0), 1.0 }, tileID);

        label->rasterize();

        std::vector<float> vertData;
        std::vector<PosTexID> bundledVertData;

        if (textBuffer->getVertices(&vertData, &nVerts)) {
            bundledVertData.resize(vertData.size() * sizeof(float)/m_vertexLayout->getStride());
            memcpy(bundledVertData.data(), vertData.data(), vertData.size()*sizeof(float));
            mesh->addVertices(std::move(bundledVertData), {});
        }

        mesh->compileVertexBuffer();

        _tile.addGeometry(*this, std::unique_ptr<VboMesh>(mesh));

        finishDataProcessing(_tile);
    }

}
