#include "labels.h"
#include "tile/mapTile.h"
#include "text/fontContext.h"

Labels::Labels() {}

Labels::~Labels() {
    m_labelUnits.clear();
    m_pendingLabelUnits.clear();
}

int Labels::LODDiscardFunc(float _maxZoom, float _zoom) {
    return (int) MIN(floor(((log(-_zoom + (_maxZoom + 2)) / log(_maxZoom + 2) * (_maxZoom )) * 0.5)), MAX_LOD);
}

bool Labels::addLabel(MapTile& _tile, const std::string& _styleName, Label::Transform _transform, std::string _text, Label::Type _type) {
    auto currentBuffer = m_ftContext->getCurrentBuffer();

    if ( (m_currentZoom - _tile.getID().z) > LODDiscardFunc(View::s_maxZoom, m_currentZoom)) {
        return false;
    }

    if (currentBuffer) {
        fsuint textID = currentBuffer->genTextID();
        std::shared_ptr<Label> l(new Label(_transform, _text, textID, _type));

        if (!l->rasterize(currentBuffer)) {
            l.reset();
            return false;
        }

        l->update(m_view->getViewProjectionMatrix() * _tile.getModelMatrix(), m_screenSize, 0);
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

void Labels::updateOcclusions() {
    m_currentZoom = m_view->getZoom();

    // merge pending labels from threads
    m_labelUnits.reserve(m_labelUnits.size() + m_pendingLabelUnits.size());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_labelUnits.insert(m_labelUnits.end(), std::make_move_iterator(m_pendingLabelUnits.begin()), std::make_move_iterator(m_pendingLabelUnits.end()));
        std::vector<LabelUnit>().swap(m_pendingLabelUnits);
    }

    std::set<std::pair<Label*, Label*>> occlusions;
    std::vector<isect2d::AABB> aabbs;

    for(size_t i = 0; i < m_labelUnits.size(); i++) {
        auto& labelUnit = m_labelUnits[i];
        auto label = labelUnit.getWeakLabel();

        if (label == nullptr) {
            m_labelUnits[i--] = std::move(m_labelUnits[m_labelUnits.size() - 1]);
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
        if(!pair.first->occludedLastFrame()) {
            if (pair.second->getState() == Label::State::wait_occ) {
                pair.second->setOcclusion(true);
            }
        }
        if(!pair.second->occludedLastFrame()) {
            if (pair.first->getState() == Label::State::wait_occ) {
                pair.first->setOcclusion(true);
            }
        }

        if(!pair.second->occludedLastFrame()) {
            pair.first->setOcclusion(true);
        }
    }

    for(size_t i = 0; i < m_labelUnits.size(); i++) {
        auto& labelUnit = m_labelUnits[i];
        auto label = labelUnit.getWeakLabel();

        if (label != nullptr) {
            label->occlusionSolved();
        }
    }
}
