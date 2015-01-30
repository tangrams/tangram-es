#include "labelContainer.h"
#include "tile/mapTile.h"

LabelContainer::LabelContainer() {}

LabelContainer::~LabelContainer() {
    m_labels.clear();
}

std::shared_ptr<Label> LabelContainer::addLabel(const std::string& _styleName, LabelTransform _transform, std::string _text) {
    auto currentBuffer = m_ftContext->getCurrentBuffer();

    if (currentBuffer) {
        std::shared_ptr<Label> label(new Label(_transform, _text, currentBuffer));
        m_labels[_styleName][processedTile->getID()].push_back(label);

        return label;
    } 

    return nullptr;
}

void LabelContainer::removeLabels(const TileID& _tileID) {
    if(m_labels.size() > 0) {
        for (auto styleTilepair : m_labels) {
            std::string styleName = styleTilepair.first;
            for (auto tileLabelsPair : m_labels[styleName]) {
                const TileID& tileID = tileLabelsPair.first;
                if (tileID == _tileID) {
                    m_labels[styleName][tileID].clear();
                }
            }
        }
    }
}

const std::vector<std::shared_ptr<Label>>& LabelContainer::getLabels(const std::string& _styleName, const TileID& _tileID) {
    return m_labels[_styleName][_tileID];
}
