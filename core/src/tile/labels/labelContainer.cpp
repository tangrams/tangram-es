#include "labelContainer.h"
#include "tile/mapTile.h"

LabelContainer::LabelContainer() {}

LabelContainer::~LabelContainer() {
    m_labelUnits.clear();
    m_pendingLabelUnits.clear();
}

bool LabelContainer::addLabel(MapTile& _tile, const std::string& _styleName, Label::Transform _transform, std::string _text, Label::Type _type) {
    auto currentBuffer = m_ftContext->getCurrentBuffer();

    if (currentBuffer) {
        fsuint textID = currentBuffer->genTextID();
        std::shared_ptr<Label> l(new Label(_transform, _text, textID, _type));
        
        l->rasterize(currentBuffer);
        l->update(m_viewProjection * _tile.getModelMatrix(), m_screenSize, 0);
        std::unique_ptr<TileID> tileID(new TileID(_tile.getID()));
        _tile.addLabel(_styleName, l);
        
        // lock concurrent collection
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pendingLabelUnits.emplace_back(LabelUnit(l, tileID, _styleName));
        }

        return true;
    }

    return false;
}

void LabelContainer::updateOcclusions() {
    // merge pending labels from threads
    m_labelUnits.reserve(m_labelUnits.size() + m_pendingLabelUnits.size());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_labelUnits.insert(m_labelUnits.end(), std::make_move_iterator(m_pendingLabelUnits.begin()), std::make_move_iterator(m_pendingLabelUnits.end()));
        std::vector<LabelUnit>().swap(m_pendingLabelUnits);
    }

    std::set<std::pair<Label*, Label*>> occlusions;
    std::vector<isect2d::AABB> aabbs;
    
    for(int i = 0; i < m_labelUnits.size(); i++) {
        auto& labelUnit = m_labelUnits[i];
        auto label = labelUnit.getWeakLabel();
        
        if (label == nullptr) {
            m_labelUnits[i] = std::move(m_labelUnits[m_labelUnits.size() - 1]);
            m_labelUnits.pop_back();
            continue;
        }
        
        if (!label->canOcclude()) {
            continue;
        }
        
        isect2d::AABB aabb = label->getAABB();
        aabb.m_userData = (void*) label.get();
        aabbs.push_back(aabb);
    }
    
    // broad phase
    auto pairs = intersect(aabbs);
    
    for (auto pair : pairs) {
        const auto& aabb1 = aabbs[pair.first];
        const auto& aabb2 = aabbs[pair.second];
        
        auto l1 = (Label*) aabb1.m_userData;
        auto l2 = (Label*) aabb2.m_userData;
        
        // narrow phase
        if (intersect(l1->getOBB(), l2->getOBB())) {
            occlusions.insert({ l1, l2 });
        }
    }
    
    // no priorities, only occlude one of the two occluded label
    for (auto& pair : occlusions) {
        if(!pair.second->occludedLastFrame()) {
            pair.first->setOcclusion(true);
        }
    }
    
    for(int i = 0; i < m_labelUnits.size(); i++) {
        auto& labelUnit = m_labelUnits[i];
        auto label = labelUnit.getWeakLabel();
        
        if (label != nullptr) {
            label->occlusionSolved();
        }
    }
}
