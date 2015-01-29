#include "labelContainer.h"
#include "tile/mapTile.h"

LabelContainer::LabelContainer() {}

LabelContainer::~LabelContainer() {}

std::shared_ptr<Label> LabelContainer::addLabel(LabelTransform _transform, std::string _text) {
    auto currentBuffer = m_ftContext->getCurrentBuffer();

    if (currentBuffer) {
        std::shared_ptr<Label> label(new Label(_transform, _text, currentBuffer));
        m_labels[processedTile->getID()].push_back(label);

        return label;
    } 

    return nullptr;
}

const std::vector<std::shared_ptr<Label>>& LabelContainer::getLabels(const TileID& _tileID) {
    return m_labels[_tileID];
}
