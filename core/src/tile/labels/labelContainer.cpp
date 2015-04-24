#include "labelContainer.h"
#include "tile/mapTile.h"

LabelContainer::LabelContainer() {}

LabelContainer::~LabelContainer() {
    m_labels.clear();
}

bool LabelContainer::addLabel(const TileID& _tileID, const std::string& _styleName, LabelTransform _transform, std::string _text, Label::Type _type) {
    auto currentBuffer = m_ftContext->getCurrentBuffer();

    if (currentBuffer) {
        std::shared_ptr<Label> label(new Label(_transform, _text, currentBuffer, _type));
        label->rasterize();
        
        m_labels[_styleName][_tileID].push_back(label);

        return true;
    } 

    return false;
}

void LabelContainer::removeLabels(const TileID& _tileID) {
    if (m_labels.size() > 0) {
        for (auto& styleTilepair : m_labels) {
            std::string styleName = styleTilepair.first;
            for (auto& tileLabelsPair : m_labels[styleName]) {
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

void LabelContainer::updateOcclusions() {
    std::set<std::pair<std::shared_ptr<Label>, std::shared_ptr<Label>>> occlusions;
    std::vector<isect2d::AABB> aabbs;
    
    for (auto& styleTilepair : m_labels) {
        std::string styleName = styleTilepair.first;
        for (auto& tileLabelsPair : m_labels[styleName]) {
            auto& labels = tileLabelsPair.second;
            for(auto& label : labels) {
                if (!label->isVisible() || label->isOutOfScreen() || label->getType() == Label::Type::DEBUG) {
                    continue;
                }
                
                isect2d::AABB aabb = label->getAABB();
                aabb.m_userData = (void*) &label;
                aabbs.push_back(aabb);
            }
        }
    }
    
    // broad phase
    auto pairs = intersect(aabbs);
    
    for (auto pair : pairs) {
        const auto& aabb1 = aabbs[pair.first];
        const auto& aabb2 = aabbs[pair.second];
        
        auto l1 = *(std::shared_ptr<Label>*) aabb1.m_userData;
        auto l2 = *(std::shared_ptr<Label>*) aabb2.m_userData;
        
        // narrow phase
        if (intersect(l1->getOBB(), l2->getOBB())) {
            occlusions.insert({ l1, l2 });
        }
    }
    
    // no priorities, only occlude one of the two occluded label
    for (auto& pair : occlusions) {
        if(pair.second->isVisible()) {
            pair.first->setVisible(false);
        }
    }
}
