#include "labels.h"
#include "tangram.h"
#include "tile/mapTile.h"
#include "text/fontContext.h"
#include "util/primitives.h"
#include "view/view.h"

#include "glm/gtc/matrix_transform.hpp"

Labels::Labels() {}

Labels::~Labels() {
    m_labels.clear();
    m_pendingLabels.clear();
}

int Labels::LODDiscardFunc(float _maxZoom, float _zoom) {
    return (int) MIN(floor(((log(-_zoom + (_maxZoom + 2)) / log(_maxZoom + 2) * (_maxZoom )) * 0.5)), MAX_LOD);
}

bool Labels::addLabel(MapTile& _tile, const std::string& _styleName, Label::Transform _transform, std::string _text, Label::Type _type) {
    auto currentBuffer = m_ftContext->getCurrentBuffer();
    if (!currentBuffer) {
        return false;
    }

    // FIXME: the current view should not be used to determine whether a label is shown at all
    // otherwise results will be random
    if ( (m_currentZoom - _tile.getID().z) > LODDiscardFunc(View::s_maxZoom, m_currentZoom)) {
        return false;
    }

    fsuint textID = currentBuffer->genTextID();
    std::shared_ptr<Label> l(new Label(_transform, _text, textID, _type));

    if (!l->rasterize(currentBuffer)) {
        l.reset();
        return false;
    }

    // NB: viewOrigin.z is only determined by screen width and height.
    const auto& viewOrigin = m_view->getPosition();

    auto modelMatrix = glm::scale(glm::mat4(1.0), glm::vec3(_tile.getScale()));
    modelMatrix[3][2] = -viewOrigin.z;

    l->update(m_view->getViewProjectionMatrix() * modelMatrix, m_screenSize, 0);
    _tile.addLabel(_styleName, l);

    // lock concurrent collection
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingLabels.emplace_back(l);
    }

    return true;
}

void Labels::updateOcclusions() {
    m_currentZoom = m_view->getZoom();

    if (!m_pendingLabels.empty()) {
        // merge pending labels from threads
        m_labels.reserve(m_labels.size() + m_pendingLabels.size());

        std::lock_guard<std::mutex> lock(m_mutex);
        m_labels.insert(m_labels.end(),
                        std::make_move_iterator(m_pendingLabels.begin()),
                        std::make_move_iterator(m_pendingLabels.end()));
        m_pendingLabels.clear();
    }

    std::set<std::pair<Label*, Label*>> occlusions;
    std::vector<isect2d::AABB> aabbs;

    for (size_t i = 0; i < m_labels.size(); i++) {
        auto label = m_labels[i].lock();

        if (!label) {
            m_labels[i--] = std::move(m_labels[m_labels.size() - 1]);
            m_labels.pop_back();
            continue;
        }

        if (!label->canOcclude()) { continue; }

        isect2d::AABB aabb = label->getAABB();
        aabb.m_userData = (void*)label.get();
        aabbs.push_back(aabb);
    }

    // broad phase
    auto pairs = intersect(aabbs, {4, 4}, {m_view->getWidth(), m_view->getHeight()});

    for (auto pair : pairs) {
        const auto& aabb1 = aabbs[pair.first];
        const auto& aabb2 = aabbs[pair.second];

        auto l1 = (Label*)aabb1.m_userData;
        auto l2 = (Label*)aabb2.m_userData;

        // narrow phase
        if (intersect(l1->getOBB(), l2->getOBB())) { occlusions.insert({l1, l2}); }
    }

    // no priorities, only occlude one of the two occluded label
    for (auto& pair : occlusions) {
        if (!pair.first->occludedLastFrame()) {
            if (pair.second->getState() == Label::State::WAIT_OCC) { pair.second->setOcclusion(true); }
        }
        if (!pair.second->occludedLastFrame()) {
            if (pair.first->getState() == Label::State::WAIT_OCC) { pair.first->setOcclusion(true); }
        }

        if (!pair.second->occludedLastFrame()) { pair.first->setOcclusion(true); }
    }

    for (size_t i = 0; i < m_labels.size(); i++) {
        auto label = m_labels[i].lock();

        if (label) { label->occlusionSolved(); }
    }
}

void Labels::drawDebug() {

    if (!Tangram::getDebugFlag(Tangram::DebugFlags::LABELS)) {
        return;
    }

    for(size_t i = 0; i < m_labels.size(); i++) {
        auto label = m_labels[i].lock();

        if (label && label->canOcclude()) {
            isect2d::OBB obb = label->getOBB();
            const isect2d::Vec2* quad = obb.getQuad();

            glm::vec2 polygon[4] = {
                {quad[0].x, quad[0].y},
                {quad[1].x, quad[1].y},
                {quad[2].x, quad[2].y},
                {quad[3].x, quad[3].y}
            };

            Primitives::drawPoly(polygon, 4, {m_view->getWidth(), m_view->getHeight()});
        }
    }

    glm::vec2 split(4, 4);
    glm::vec2 res(m_view->getWidth(), m_view->getHeight());
    const short xpad = short(ceilf(res.x / split.x));
    const short ypad = short(ceilf(res.y / split.y));

    short x = 0, y = 0;
    for (int j = 0; j < split.y; ++j) {
        for (int i = 0; i < split.x; ++i) {
            isect2d::AABB cell(x, y, x + xpad, y + ypad);
            Primitives::drawRect({x, y}, {x + xpad, y + ypad}, res);
            x += xpad;
            if (x >= res.x) {
                x = 0;
                y += ypad;
            }
        }
    }

}
