#include "labelContainer.h"
#include "tile/mapTile.h"

LabelContainer::LabelContainer() {}

LabelContainer::~LabelContainer() {
    m_labels.clear();
    m_pendingLabels.clear();
}

bool LabelContainer::addLabel(const TileID& _tileID, const std::string& _styleName, LabelTransform _transform, std::string _text, Label::Type _type, const glm::mat4& _model) {
    auto currentBuffer = m_ftContext->getCurrentBuffer();

    if (currentBuffer) {
        auto& container = m_pendingLabels[_styleName][_tileID];
        
        // lock concurrent collection
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            container.emplace_back(_transform, _text, currentBuffer, _type);
            
            container.back().rasterize();
            
            // ensure label is updated once
            container.back().update(m_viewProjection * _model, m_screenSize, 0);
        }

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
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_pendingLabels[styleName][tileID].clear();
                    }
                }
            }
        }
    }
}

const std::vector<std::shared_ptr<Label>>& LabelContainer::getLabels(const std::string& _styleName, const TileID& _tileID) {
    return m_labels[_styleName][_tileID];
}

void LabelContainer::updateOcclusions() {
    // merge pending labels from threads
    for (auto& styleTilepair : m_pendingLabels) {
        std::string styleName = styleTilepair.first;
        
        for (auto& tileLabelsPair : m_pendingLabels[styleName]) {
            const TileID& tileID = tileLabelsPair.first;
            auto& pendingLabels = m_pendingLabels[styleName][tileID];
            auto& labels = m_labels[styleName][tileID];
            
            for (auto& label : pendingLabels) {
                // create a shared pointer as a copy of the label created in the thread
                labels.emplace_back(new Label(label));
            }
            
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                pendingLabels.clear();
            }
        }
    }
    
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
