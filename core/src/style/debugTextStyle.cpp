#include "debugTextStyle.h"

#include "text/fontContext.h"
#include "tile/mapTile.h"
#include "util/vboMesh.h"

DebugTextStyle::DebugTextStyle(const std::string& _fontName, std::string _name, float _fontSize,
                               unsigned int _color, bool _sdf, GLenum _drawMode)
    : TextStyle(_fontName, _name, _fontSize, _color, _sdf, false, _drawMode) {
}

void DebugTextStyle::addData(TileData& _data, MapTile& _tile) {

    if (Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
        auto batch = static_cast<TextBatch*>(newBatch());

        auto ftContext = m_labels->getFontContext();

        ftContext->setFont(m_fontName, m_fontSize * m_pixelScale);

        if (m_sdf) {
            float blurSpread = 2.5;
            ftContext->setSignedDistanceField(blurSpread);
        }

        std::string tileID = std::to_string(_tile.getID().x) + "/" +
                             std::to_string(_tile.getID().y) + "/" +
                             std::to_string(_tile.getID().z);

        batch->addLabel(m_labels->addTextLabel(*batch, _tile, { glm::vec2(0), glm::vec2(0) }, tileID, Label::Type::debug));

        if (batch->compile()) {
            _tile.addBatch(*this, std::unique_ptr<StyleBatch>(batch));
        }
    }
}
