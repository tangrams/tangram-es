#include "labels.h"
#include "tangram.h"
#include "tile/tile.h"
#include "text/fontContext.h"
#include "gl/primitives.h"
#include "view/view.h"

#include "glm/gtc/matrix_transform.hpp"

Labels::Labels() {}

Labels::~Labels() {}

int Labels::LODDiscardFunc(float _maxZoom, float _zoom) {
    return (int) MIN(floor(((log(-_zoom + (_maxZoom + 2)) / log(_maxZoom + 2) * (_maxZoom )) * 0.5)), MAX_LOD);
}

std::shared_ptr<Label> Labels::addTextLabel(Tile& _tile, TextBuffer& _buffer, const std::string& _styleName,
                                            Label::Transform _transform, std::string _text, Label::Type _type) {
    // FIXME: the current view should not be used to determine whether a label is shown at all
    // otherwise results will be random

    // discard based on level of detail
    if ((m_currentZoom - _tile.getID().z) > LODDiscardFunc(View::s_maxZoom, m_currentZoom)) {
        return nullptr;
    }

    fsuint textID = _buffer.genTextID();

    std::shared_ptr<TextLabel> label(new TextLabel(_transform, _text, textID, _type));

    // raterize the text label
    if (!label->rasterize(_buffer)) {

        label.reset();
        return nullptr;
    }

    addLabel(_tile, _styleName, label);

    return label;
}

std::shared_ptr<Label> Labels::addSpriteLabel(Tile& _tile, const std::string& _styleName, Label::Transform _transform,
                                              const glm::vec2& _size, size_t _bufferOffset) {

    if ((m_currentZoom - _tile.getID().z) > LODDiscardFunc(View::s_maxZoom, m_currentZoom)) {
        return nullptr;
    }

    auto label = std::shared_ptr<Label>(new SpriteLabel(_transform, _size, _bufferOffset));
    addLabel(_tile, _styleName, label);

    return label;
}

void Labels::addLabel(Tile& _tile, const std::string& _styleName, std::shared_ptr<Label> _label) {

    auto modelMatrix = glm::scale(glm::mat4(1.0), glm::vec3(_tile.getScale()));
    // NB: viewOrigin.z is only determined by screen width and height.
    const auto& viewOrigin = m_view->getPosition();
    modelMatrix[3][2] = -viewOrigin.z;

    _label->update(m_view->getViewProjectionMatrix() * modelMatrix, {m_view->getWidth(), m_view->getHeight()}, 0);
    _tile.addLabel(_styleName, _label);

    // lock concurrent collection
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingLabels.push_back(_label);
    }
}

void Labels::updateOcclusions() {
    m_currentZoom = m_view->getZoom();

    // merge pending labels from threads
    if (!m_pendingLabels.empty()) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_labels.reserve(m_labels.size() + m_pendingLabels.size());

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
        if(!pair.first->occludedLastFrame()) {
            if (pair.second->getState() == Label::State::wait_occ) { pair.second->setOcclusion(true); }
        }
        if(!pair.second->occludedLastFrame()) {
            if (pair.first->getState() == Label::State::wait_occ) { pair.first->setOcclusion(true); }
        }

        if (!pair.second->occludedLastFrame()) { pair.first->setOcclusion(true); }
    }

    for (auto& labelPtr : m_labels) {
        auto label = labelPtr.lock();
        if (label) { label->occlusionSolved(); }
    }
}

void Labels::drawDebug() {

    if (!Tangram::getDebugFlag(Tangram::DebugFlags::labels)) {
        return;
    }

    for(auto& labelPtr : m_labels) {
        auto label = labelPtr.lock();

        if (label && label->canOcclude()) {
            Primitives::drawPoly(reinterpret_cast<const glm::vec2*>(label->getOBB().getQuad()),
                                 4, { m_view->getWidth(), m_view->getHeight() });
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
