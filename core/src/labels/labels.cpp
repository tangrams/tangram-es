#include "labels.h"

#include "tangram.h"
#include "platform.h"
#include "gl/shaderProgram.h"
#include "gl/primitives.h"
#include "view/view.h"
#include "style/style.h"
#include "style/pointStyle.h"
#include "style/textStyle.h"
#include "tile/tile.h"
#include "tile/tileCache.h"
#include "labels/labelSet.h"
#include "labels/textLabel.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/norm.hpp"

namespace Tangram {

Labels::Labels()
    : m_needUpdate(false),
      m_lastZoom(0.0f) {}

Labels::~Labels() {}

// int Labels::LODDiscardFunc(float _maxZoom, float _zoom) {
//     return (int) MIN(floor(((log(-_zoom + (_maxZoom + 2)) / log(_maxZoom + 2) * (_maxZoom )) * 0.5)), MAX_LOD);
// }

void Labels::updateLabels(const View& _view, float _dt,
                          const std::vector<std::unique_ptr<Style>>& _styles,
                          const std::vector<std::shared_ptr<Tile>>& _tiles,
                          bool _onlyTransitions) {

    m_labels.clear();

    m_needUpdate = false;

    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());

    // int lodDiscard = LODDiscardFunc(View::s_maxZoom, _view.getZoom());
    float dz = _view.getZoom() - std::floor(_view.getZoom());

    bool allLabels = Tangram::getDebugFlag(DebugFlags::all_labels);

    for (const auto& tile : _tiles) {

        // discard based on level of detail
        // if ((zoom - tile->getID().z) > lodDiscard) {
        //     continue;
        // }

        bool proxyTile = tile->isProxy();

        glm::mat4 mvp = _view.getViewProjectionMatrix() * tile->getModelMatrix();

        for (const auto& style : _styles) {
            const auto& mesh = tile->getMesh(*style);
            if (!mesh) { continue; }

            auto labelMesh = dynamic_cast<const LabelSet*>(mesh.get());
            if (!labelMesh) { continue; }
            for (auto& label : labelMesh->getLabels()) {
                if (!label->update(mvp, screenSize, dz, allLabels)) {
                    // skip dead labels
                    continue;
                }

                if (_onlyTransitions) {
                    if (!label->canOcclude() || label->visibleState()) {
                        m_needUpdate |= label->evalState(screenSize, _dt);
                        label->pushTransform();
                    }
                } else if (label->canOcclude()) {
                    if (label->state() != Label::State::out_of_screen) {
                        m_labels.emplace_back(label.get(), proxyTile);
                    }
                } else {
                    m_needUpdate |= label->evalState(screenSize, _dt);
                    label->pushTransform();
                }
            }
        }
    }
}

void Labels::skipTransitions(const std::vector<const Style*>& _styles, Tile& _tile, Tile& _proxy) const {

    for (const auto& style : _styles) {

        auto* mesh0 = dynamic_cast<const LabelSet*>(_tile.getMesh(*style).get());
        if (!mesh0) { continue; }

        auto* mesh1 = dynamic_cast<const LabelSet*>(_proxy.getMesh(*style).get());
        if (!mesh1) { continue; }

        for (auto& l0 : mesh0->getLabels()) {
            if (!l0->canOcclude()) { continue; }
            if (l0->state() != Label::State::wait_occ) { continue; }

            for (auto& l1 : mesh1->getLabels()) {
                if (!l1->visibleState()) { continue; }
                if (!l1->canOcclude()) { continue;}

                // Using repeat group to also handle labels with dynamic style properties
                if (l0->options().repeatGroup != l1->options().repeatGroup) { continue; }
                // if (l0->hash() != l1->hash()) { continue; }

                float d2 = glm::distance2(l0->transform().state.screenPos,
                                          l1->transform().state.screenPos);

                // The new label lies within the circle defined by the bbox of l0
                if (sqrt(d2) < std::max(l0->dimension().x, l0->dimension().y)) {
                    l0->skipTransitions();
                }
            }
        }
    }
}

std::shared_ptr<Tile> findProxy(int32_t _sourceID, const TileID& _proxyID,
                                const std::vector<std::shared_ptr<Tile>>& _tiles,
                                std::unique_ptr<TileCache>& _cache) {

    auto proxy = _cache->contains(_sourceID, _proxyID);
    if (proxy) { return proxy; }

    for (auto& tile : _tiles) {
        if (tile->getID() == _proxyID && tile->sourceID() == _sourceID) {
            return tile;
        }
    }
    return nullptr;
}

void Labels::skipTransitions(const std::vector<std::unique_ptr<Style>>& _styles,
                             const std::vector<std::shared_ptr<Tile>>& _tiles,
                             std::unique_ptr<TileCache>& _cache, float _currentZoom) const {

    std::vector<const Style*> styles;

    for (const auto& style : _styles) {
        if (dynamic_cast<const TextStyle*>(style.get()) ||
            dynamic_cast<const PointStyle*>(style.get())) {
            styles.push_back(style.get());
        }
    }

    for (const auto& tile : _tiles) {
        TileID tileID = tile->getID();
        std::shared_ptr<Tile> proxy;

        if (m_lastZoom < _currentZoom) {
            // zooming in, add the one cached parent tile
            proxy = findProxy(tile->sourceID(), tileID.getParent(), _tiles, _cache);
            if (proxy) { skipTransitions(styles, *tile, *proxy); }
        } else {
            // zooming out, add the 4 cached children tiles
            proxy = findProxy(tile->sourceID(), tileID.getChild(0), _tiles, _cache);
            if (proxy) { skipTransitions(styles, *tile, *proxy); }

            proxy = findProxy(tile->sourceID(), tileID.getChild(1), _tiles, _cache);
            if (proxy) { skipTransitions(styles, *tile, *proxy); }

            proxy = findProxy(tile->sourceID(), tileID.getChild(2), _tiles, _cache);
            if (proxy) { skipTransitions(styles, *tile, *proxy); }

            proxy = findProxy(tile->sourceID(), tileID.getChild(3), _tiles, _cache);
            if (proxy) { skipTransitions(styles, *tile, *proxy); }
        }
    }
}

bool Labels::labelComparator(const LabelEntry& _a, const LabelEntry& _b) {
    if (_a.proxy != _b.proxy) {
        return _b.proxy;
    }

    if (_a.priority != _b.priority) {
        return _a.priority < _b.priority;
    }

    auto l1 = _a.label;
    auto l2 = _b.label;

    // Note: This causes non-deterministic placement, i.e. depending on
    // navigation history.
    if (l1->occludedLastFrame() != l2->occludedLastFrame()) {
        return l2->occludedLastFrame();
    }

    // if (l1->options().repeatGroup != l2->options().repeatGroup) {
    //     return l1->options().repeatGroup < l2->options().repeatGroup;
    // }

    if (l1->type() == Label::Type::line && l2->type() == Label::Type::line) {
        // Prefer the label with longer line segment as it has a chance
        return glm::length2(l1->transform().modelPosition1 - l1->transform().modelPosition2) >
               glm::length2(l2->transform().modelPosition1 - l2->transform().modelPosition2);
    }

    if (l1->hash() != l2->hash()) {
        return l1->hash() < l2->hash();
    }

    return l1 < l2;
}

void Labels::sortLabels() {
    std::sort(m_labels.begin(), m_labels.end(), Labels::labelComparator);
}

void Labels::handleOcclusions() {

    m_isect2d.clear();
    m_repeatGroups.clear();

    for (auto& entry : m_labels){
        auto* l = entry.label;

        // Parent must have been processed earlier so at this point
        // its occlusion is determined for the current frame.
        if (l->parent() && l->parent()->isOccluded()) {
            l->occlude();
            continue;
        }

        // Skip label if another label of this repeatGroup is
        // within repeatDistance.
        if (l->options().repeatDistance > 0.f) {
            if (withinRepeatDistance(l)) {
                l->occlude();
                continue;
            }
        }

        // Skip label if it intersects with a previous label.
        auto aabb = l->aabb();
        aabb.m_userData = static_cast<void*>(l);

        m_isect2d.intersect(aabb, [](auto& a, auto& b) {
                auto* l1 = static_cast<Label*>(a.m_userData);
                auto* l2 = static_cast<Label*>(b.m_userData);

                if (intersect(l1->obb(), l2->obb())) {
                    l1->occlude();
                    // Drop label
                    return false;
                }
                // Continue
                return true;
            });

        if (l->options().repeatDistance > 0.f) {
            m_repeatGroups[l->options().repeatGroup].push_back(l);
        }
    }
}

bool Labels::withinRepeatDistance(Label *_label) {
    float threshold2 = pow(_label->options().repeatDistance, 2);

    auto it = m_repeatGroups.find(_label->options().repeatGroup);
    if (it != m_repeatGroups.end()) {
        for (auto* ll : it->second) {
            float d2 = distance2(_label->center(), ll->center());
            if (d2 < threshold2) {
                return true;
            }
        }
    }
    return false;
}

void Labels::updateLabelSet(const View& _view, float _dt,
                            const std::vector<std::unique_ptr<Style>>& _styles,
                            const std::vector<std::shared_ptr<Tile>>& _tiles,
                            std::unique_ptr<TileCache>& _cache) {

    /// Collect and update labels from visible tiles
    updateLabels(_view, _dt, _styles, _tiles, false);

    sortLabels();

    /// Mark labels to skip transitions

    if (int(m_lastZoom) != int(_view.getZoom())) {
        skipTransitions(_styles, _tiles, _cache, _view.getZoom());
        m_lastZoom = _view.getZoom();
    }

    m_isect2d.resize({_view.getWidth() / 256, _view.getHeight() / 256},
                     {_view.getWidth(), _view.getHeight()});

    handleOcclusions();

    /// Update label meshes

    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());

    for (auto& entry : m_labels) {
        Label* label = entry.label;

        m_needUpdate |= label->evalState(screenSize, _dt);
        label->pushTransform();
    }
}

const std::vector<TouchItem>& Labels::getFeaturesAtPoint(const View& _view, float _dt,
                                                         const std::vector<std::unique_ptr<Style>>& _styles,
                                                         const std::vector<std::shared_ptr<Tile>>& _tiles,
                                                         float _x, float _y, bool _visibleOnly) {
    // FIXME dpi dependent threshold
    const float thumbSize = 50;

    m_touchItems.clear();

    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());
    glm::vec2 touchPoint(_x, _y);

    OBB obb(_x - thumbSize/2, _y - thumbSize/2, 0, thumbSize, thumbSize);

    float z = _view.getZoom();
    float dz = z - std::floor(z);

    for (const auto& tile : _tiles) {

        glm::mat4 mvp = _view.getViewProjectionMatrix() * tile->getModelMatrix();

        for (const auto& style : _styles) {
            const auto& mesh = tile->getMesh(*style);
            if (!mesh) { continue; }

            auto labelMesh = dynamic_cast<const LabelSet*>(mesh.get());
            if (!labelMesh) { continue; }

            for (auto& label : labelMesh->getLabels()) {

                auto& options = label->options();
                if (!options.interactive) { continue; }

                if (!_visibleOnly) {
                    label->updateScreenTransform(mvp, screenSize, false);
                    label->updateBBoxes(dz);
                } else if (!label->visibleState()) {
                    continue;
                }

                if (isect2d::intersect(label->obb(), obb)) {
                    float distance = glm::length2(label->transform().state.screenPos - touchPoint);
                    auto labelCenter = label->center();
                    m_touchItems.push_back({options.properties, {labelCenter.x, labelCenter.y}, std::sqrt(distance)});
                }
            }
        }
    }

    std::sort(m_touchItems.begin(), m_touchItems.end(),
              [](auto& a, auto& b){ return a.distance < b.distance; });


    return m_touchItems;
}

void Labels::drawDebug(const View& _view) {

    if (!Tangram::getDebugFlag(Tangram::DebugFlags::labels)) {
        return;
    }

    for (auto& entry : m_labels) {
        auto* label = entry.label;

        if (label->type() == Label::Type::debug) { continue; }

        glm::vec2 sp = label->transform().state.screenPos;

        // draw bounding box
        switch (label->state()) {
        case Label::State::sleep:
            Primitives::setColor(0x00ff00);
            break;
        case Label::State::visible:
            Primitives::setColor(0x000000);
            break;
        case Label::State::wait_occ:
            Primitives::setColor(0x0000ff);
            break;
        case Label::State::dead:
            Primitives::setColor(0xff00ff);
            break;
        case Label::State::fading_in:
        case Label::State::fading_out:
            Primitives::setColor(0xffff00);
            break;
        default:
            Primitives::setColor(0x999999);
        }

        Primitives::drawPoly(&(label->obb().getQuad())[0], 4);

        if (label->type() == Label::Type::line) {
            Primitives::drawLine(label->transform().screenPosition1, label->transform().screenPosition2);
        }

        if (label->visibleState() && label->parent()) {
            Primitives::setColor(0xff0000);
            Primitives::drawLine(sp, label->parent()->transform().state.screenPos);
        }

        // draw offset
        glm::vec2 rot = label->transform().state.rotation;
        glm::vec2 offset = rotateBy(label->options().offset, rot);
        Primitives::setColor(0x000000);
        Primitives::drawLine(sp, sp - glm::vec2(offset.x, -offset.y));

        // draw projected anchor point
        Primitives::setColor(0x0000ff);
        Primitives::drawRect(sp - glm::vec2(1.f), sp + glm::vec2(1.f));
#if 0
        if (label->options().repeatGroup != 0 && label->state() == Label::State::visible) {
            size_t seed = 0;
            hash_combine(seed, label->options().repeatGroup);
            float repeatDistance = label->options().repeatDistance;

            Primitives::setColor(seed);
            Primitives::drawLine(label->center(),
                                 glm::vec2(repeatDistance, 0.f) + label->center());

            float off = M_PI / 6.f;
            for (float pad = 0.f; pad < M_PI * 2.f; pad += off) {
                glm::vec2 p0 = glm::vec2(cos(pad), sin(pad)) * repeatDistance
                    + label->center();
                glm::vec2 p1 = glm::vec2(cos(pad + off), sin(pad + off)) * repeatDistance
                    + label->center();
                Primitives::drawLine(p0, p1);
            }
        }
#endif
    }

    glm::vec2 split(_view.getWidth() / 256, _view.getHeight() / 256);
    glm::vec2 res(_view.getWidth(), _view.getHeight());
    const short xpad = short(ceilf(res.x / split.x));
    const short ypad = short(ceilf(res.y / split.y));

    Primitives::setColor(0x7ef586);
    short x = 0, y = 0;
    for (int j = 0; j < split.y; ++j) {
        for (int i = 0; i < split.x; ++i) {
            AABB cell(x, y, x + xpad, y + ypad);
            Primitives::drawRect({x, y}, {x + xpad, y + ypad});
            x += xpad;
            if (x >= res.x) {
                x = 0;
                y += ypad;
            }
        }
    }
}

}
