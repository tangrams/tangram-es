#include "labelContainer.h"
#include "tile/mapTile.h"

LabelContainer::LabelContainer() {}

LabelContainer::~LabelContainer() {
    m_labelUnits.clear();
    m_pendingLabelUnits.clear();
}

bool LabelContainer::addLabel(MapTile& _tile, const std::string& _styleName, LabelTransform _transform, std::string _text, Label::Type _type) {
    
    auto currentBuffer = m_ftContext->getCurrentBuffer();

    if (currentBuffer) {
        std::shared_ptr<Label> l(new Label(_transform, _text, currentBuffer, _type));
        
        l->rasterize();
        l->update(m_viewProjection * _tile.getModelMatrix(), m_screenSize, 0);
        std::unique_ptr<TileID> tileID(new TileID(_tile.getID()));
        _tile.addLabel(l);
        
        // lock concurrent collection
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pendingLabelUnits.emplace_back(LabelUnit(l, tileID, _styleName));
        }

        return true;
    }

    return false;
}

void LabelContainer::removeLabels(const TileID& _tileID) {
    if (m_labelUnits.size() > 0) {
        for(size_t i = 0; i < m_labelUnits.size(); i++) {
            if(*(m_labelUnits[i].m_tileID) == _tileID) {
                m_labelUnits[i] = std::move(m_labelUnits[m_labelUnits.size() - 1]);
                m_labelUnits.pop_back();
            }
        }
        
        for(size_t i = 0; i < m_pendingLabelUnits.size(); i++) {
            if(*(m_pendingLabelUnits[i].m_tileID) == _tileID) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_pendingLabelUnits[i] = std::move(m_pendingLabelUnits[m_pendingLabelUnits.size() - 1]);
                m_pendingLabelUnits.pop_back();
            }
        }
    }
}

void LabelContainer::updateOcclusions() {
    // merge pending labels from threads
    m_labelUnits.reserve(m_labelUnits.size() + m_pendingLabelUnits.size());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_labelUnits.insert(m_labelUnits.end(), std::make_move_iterator(m_pendingLabelUnits.begin()), std::make_move_iterator(m_pendingLabelUnits.end()));
        std::vector<LabelUnit>().swap(m_pendingLabelUnits);
    }

    std::set<std::pair<std::shared_ptr<Label>, std::shared_ptr<Label>>> occlusions;
    std::vector<isect2d::AABB> aabbs;

    for(auto& labelUnit : m_labelUnits) {
        auto& label = labelUnit.m_label;
        if (!label->isVisible() || label->isOutOfScreen() || label->getType() == Label::Type::DEBUG) {
            continue;
        }

        isect2d::AABB aabb = label->getAABB();
        aabb.m_userData = (void*) &label;
        aabbs.push_back(aabb);
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
