#include "labels/labelManager.h"

#include "data/tileSource.h"
#include "gl/primitives.h"
#include "gl/shaderProgram.h"
#include "labels/curvedLabel.h"
#include "labels/labelSet.h"
#include "labels/obbBuffer.h"
#include "labels/textLabel.h"
#include "map.h"
#include "marker/marker.h"
#include "platform.h"
#include "scene/scene.h"
#include "style/pointStyle.h"
#include "style/style.h"
#include "style/textStyle.h"
#include "tile/tile.h"
#include "tile/tileCache.h"
#include "tile/tileManager.h"
#include "view/view.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/norm.hpp"

#include <cassert>

namespace Tangram {

LabelManager::LabelManager()
    : m_needUpdate(false),
      m_lastZoom(0.0f) {}

LabelManager::~LabelManager() {}

void LabelManager::processLabelUpdate(const ViewState& _viewState, const LabelSet* _labelSet, Style* _style,
                                const Tile* _tile, const Marker* _marker, const glm::mat4& _mvp,
                                float _dt, bool _drawAll, bool _onlyRender, bool _isProxy) {

    // TODO appropriate buffer to filter out-of-screen labels
    float border = 256.0f;
    AABB extendedBounds(-border, -border,
                        _viewState.viewportSize.x + border,
                        _viewState.viewportSize.y + border);

    AABB screenBounds(0, 0,
                      _viewState.viewportSize.x,
                      _viewState.viewportSize.y);

    for (auto& label : _labelSet->getLabels()) {
        if (!_drawAll && (label->state() == Label::State::dead) ) {
            continue;
        }

        Range transformRange;
        ScreenTransform transform { m_transforms, transformRange };

        // Use extendedBounds when labels take part in collision detection.
        auto bounds = (_onlyRender || !label->canOcclude())
            ? screenBounds
            : extendedBounds;

        if (!label->update(_mvp, _viewState, &bounds, transform)) {
            continue;
        }


        if (_onlyRender) {
            if (label->occludedLastFrame()) { label->occlude(); }

            if (label->visibleState() || !label->canOcclude()) {
                m_needUpdate |= label->evalState(_dt);
                label->addVerticesToMesh(transform, _viewState.viewportSize);
            }
        } else if (label->canOcclude()) {
            m_labels.emplace_back(label.get(), _style, _tile, _marker, _isProxy, transformRange);
        } else {
            m_needUpdate |= label->evalState(_dt);
            label->addVerticesToMesh(transform, _viewState.viewportSize);
        }
        if (label->selectionColor()) {
            m_selectionLabels.emplace_back(label.get(), _style, _tile, _marker, _isProxy, transformRange);
        }
    }
}

std::pair<Label*, const Tile*> LabelManager::getLabel(uint32_t _selectionColor) const {

    for (auto& entry : m_selectionLabels) {

        if (entry.label->visibleState() &&
            entry.label->selectionColor() == _selectionColor) {

            return { entry.label, entry.tile };
        }
    }
    return {nullptr, nullptr};
}

void LabelManager::updateLabels(const ViewState& _viewState, float _dt,
                          const std::vector<std::unique_ptr<Style>>& _styles,
                          const std::vector<std::shared_ptr<Tile>>& _tiles,
                          const std::vector<std::unique_ptr<Marker>>& _markers,
                          bool _onlyRender) {

    if (!_onlyRender) { m_labels.clear(); }

    m_selectionLabels.clear();

    m_needUpdate = false;

    // int lodDiscard = LODDiscardFunc(View::s_maxZoom, _view.getZoom());

    bool drawAllLabels = Tangram::getDebugFlag(DebugFlags::draw_all_labels);

    for (const auto& tile : _tiles) {

        //LOG("tile: %d/%d z:%d,%d", tile->getID().x, tile->getID().y, tile->getID().z, tile->getID().s);

        // discard based on level of detail
        // if ((zoom - tile->getID().z) > lodDiscard) {
        //     continue;
        // }

        bool proxyTile = tile->isProxy();

        glm::mat4 mvp = tile->mvp();

        for (const auto& style : _styles) {
            const auto& mesh = tile->getMesh(*style);
            auto labels = dynamic_cast<const LabelSet*>(mesh.get());
            if (!labels) { continue; }

            processLabelUpdate(_viewState, labels, style.get(), tile.get(), nullptr, mvp,
                               _dt, drawAllLabels, _onlyRender, proxyTile);
        }
    }

    for (const auto& marker : _markers) {

        if (!marker->isVisible() || !marker->mesh()) { continue; }

        for (const auto& style : _styles) {

            if (marker->styleId() != style->getID()) { continue; }

            const auto& mesh = marker->mesh();
            auto labels = dynamic_cast<const LabelSet*>(mesh);
            if (!labels) { continue; }

            processLabelUpdate(_viewState, labels, style.get(), nullptr, marker.get(),
                               marker->modelViewProjectionMatrix(),
                               _dt, drawAllLabels, _onlyRender, false);
        }
    }
}

void LabelManager::skipTransitions(const std::vector<const Style*>& _styles, Tile& _tile, Tile& _proxy) const {

    for (const auto& style : _styles) {

        auto* mesh0 = dynamic_cast<const LabelSet*>(_tile.getMesh(*style).get());
        if (!mesh0) { continue; }

        auto* mesh1 = dynamic_cast<const LabelSet*>(_proxy.getMesh(*style).get());
        if (!mesh1) { continue; }

        for (auto& l0 : mesh0->getLabels()) {
            if (!l0->canOcclude()) { continue; }
            if (l0->state() != Label::State::none) { continue; }

            for (auto& l1 : mesh1->getLabels()) {
                if (!l1->visibleState()) { continue; }
                if (!l1->canOcclude()) { continue;}

                // Using repeat group to also handle labels with dynamic style properties
                if (l0->options().repeatGroup != l1->options().repeatGroup) { continue; }
                // if (l0->hash() != l1->hash()) { continue; }

                float d2 = glm::distance2(l0->screenCenter(), l1->screenCenter());

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
                                TileCache& _cache) {

    auto proxy = _cache.contains(_sourceID, _proxyID);
    if (proxy) { return proxy; }

    for (auto& tile : _tiles) {
        if (tile->getID() == _proxyID && tile->sourceID() == _sourceID) {
            return tile;
        }
    }
    return nullptr;
}

void LabelManager::skipTransitions(const Scene& _scene, const std::vector<std::shared_ptr<Tile>>& _tiles,
                                   TileManager& _tileManager, float _currentZoom) const {

    std::vector<const Style*> styles;

    for (const auto& style : _scene.styles()) {
        if (dynamic_cast<const TextStyle*>(style.get()) ||
            dynamic_cast<const PointStyle*>(style.get())) {
            styles.push_back(style.get());
        }
    }

    for (const auto& tile : _tiles) {
        TileID tileID = tile->getID();
        std::shared_ptr<Tile> proxy;

        auto source = _scene.getTileSource(tile->sourceID());
        if (!source) {
            source = _tileManager.getClientTileSource(tile->sourceID());
            // If tiles for this source exist, this source must exist (either tile or client source)
            assert(source);
            continue;
        }

        if (m_lastZoom < _currentZoom) {
            // zooming in, add the one cached parent tile
            proxy = findProxy(tile->sourceID(), tileID.getParent(source->zoomBias()), _tiles,
                              *_tileManager.getTileCache());
            if (proxy) { skipTransitions(styles, *tile, *proxy); }
        } else {
            // zooming out, add the 4 cached children tiles
            for (int i = 0; i < 4; i++) {
                proxy = findProxy(tile->sourceID(), tileID.getChild(i, source->maxZoom()), _tiles,
                                  *_tileManager.getTileCache());
                if (proxy) { skipTransitions(styles, *tile, *proxy); }
            }
        }
    }
}

bool LabelManager::priorityComparator(const LabelEntry& _a, const LabelEntry& _b) {
    if (_a.proxy != _b.proxy) {
        return _b.proxy;
    }
    if (_a.priority != _b.priority) {
        return _a.priority < _b.priority;
    }
    if (!_a.tile || !_b.tile) {
        return (bool)_a.tile;
    }
    if (_a.tile->getID().z != _b.tile->getID().z) {
        return _a.tile->getID().z > _b.tile->getID().z;
    }

    auto l1 = _a.label;
    auto l2 = _b.label;

    // Note: This causes non-deterministic placement, i.e. depending on
    // navigation history.
    if (l1->occludedLastFrame() != l2->occludedLastFrame()) {
        return l2->occludedLastFrame();
    }
    // This prefers labels within screen over out_of_screen.
    // Important for repeat groups!
    if (l1->visibleState() != l2->visibleState()) {
        return l1->visibleState();
    }

    if (l1->options().repeatGroup != l2->options().repeatGroup) {
        return l1->options().repeatGroup < l2->options().repeatGroup;
    }

    if (l1->type() == l2->type()) {
        return l1->candidatePriority() < l2->candidatePriority();
    }

    if (l1->hash() != l2->hash()) {
        return l1->hash() < l2->hash();
    }

    return l1 < l2;
}

bool LabelManager::zOrderComparator(const LabelEntry& _a, const LabelEntry& _b) {

    if (_a.style != _b.style) {
        return _a.style < _b.style;
    }

    if (_a.marker && _b.marker) {
        if (_a.marker->drawOrder() != _b.marker->drawOrder()) {
            return _a.marker->drawOrder() < _b.marker->drawOrder();
        }
    }

    // Sort by texture to reduce draw calls (increase batching)
    if (_a.label->texture() != _b.label->texture()) {
        return _a.label->texture() < _b.label->texture();
    }

    // Sort Markers by id
    if (_a.marker && _b.marker) {
        return _a.marker->id() < _b.marker->id();
    }

    // Just keep tile label order consistent
    if (_a.tile && _b.tile) {
        return _a.label < _b.label;
    }

    // Add tile labels before markers
    return bool(_a.tile);
}

void LabelManager::handleOcclusions(const ViewState& _viewState) {

    m_isect2d.clear();
    m_repeatGroups.clear();

    using iterator = decltype(m_labels)::const_iterator;

    // Find the label to which the obb belongs
    auto findLabel = [](iterator begin, iterator end, int obb) {
        for (auto it = begin; it != end; it++) {
            if (obb >= it->obbsRange.start && obb < it->obbsRange.end()) {
                return it->label;
            }
        }
        assert(false);
        return static_cast<Label*>(nullptr);
    };

    for (auto it = m_labels.begin(); it != m_labels.end(); ++it) {
        auto& entry = *it;
        auto* l = entry.label;

        ScreenTransform transform { m_transforms, entry.transformRange };
        OBBBuffer obbs { m_obbs, entry.obbsRange };

        l->obbs(transform, obbs);

        // Parent must have been processed earlier so at this point its
        // occlusion and anchor position is determined for the current frame.
        if (l->isChild()) {
            if (l->relative()->isOccluded()) {
                l->occlude();
                continue;
            }
        }

        // Skip label if another label of this repeatGroup is
        // within repeatDistance.
        if (l->options().repeatDistance > 0.f) {
            if (withinRepeatDistance(l)) {
                l->occlude();
                // If this label is not marked optional, then mark the relative label as occluded
                if (l->relative() && !l->options().optional) {
                    l->relative()->occlude();
                }
                continue;
            }
        }

        int anchorIndex = l->anchorIndex();

        // For each anchor
        do {
            if (l->isOccluded()) {
                // Update OBB for anchor fallback
                obbs.clear();

                l->obbs(transform, obbs);

                if (anchorIndex == l->anchorIndex()) {
                    // Reached first anchor again
                    break;
                }
            }

            l->occlude(false);

            // Occlude label when its obbs intersect with a previous label.
            for (auto& obb : obbs) {
                m_isect2d.intersect(obb.getExtent(), [&](auto& a, auto& b) {
                        size_t other = reinterpret_cast<size_t>(b.m_userData);

                        if (!intersect(obb, m_obbs[other])) {
                            return true;
                        }
                        // Ignore intersection with relative label
                        if (l->relative() && l->relative() == findLabel(std::begin(m_labels), it, other)) {
                            return true;
                        }
                        l->occlude();
                        return false;

                    }, false);

                if (l->isOccluded()) { break; }
            }
        } while (l->isOccluded() && l->nextAnchor());

        // At this point, the label has a relative that is visible,
        // if it is not an optional label, turn the relative to occluded
        if (l->isOccluded()) {
            if (l->relative() && !l->options().optional) {
                l->relative()->occlude();
            }
        } else {
            // Insert into ISect2D grid
            int obbPos = entry.obbsRange.start;
            for (auto& obb : obbs) {
                auto aabb = obb.getExtent();
                aabb.m_userData = reinterpret_cast<void*>(obbPos++);
                m_isect2d.insert(aabb);
            }

            if (l->options().repeatDistance > 0.f) {
                m_repeatGroups[l->options().repeatGroup].push_back(l);
            }
        }
    }
}

bool LabelManager::withinRepeatDistance(Label *_label) {
    float threshold2 = pow(_label->options().repeatDistance, 2);

    auto it = m_repeatGroups.find(_label->options().repeatGroup);
    if (it != m_repeatGroups.end()) {
        for (auto* ll : it->second) {
            float d2 = glm::distance2(_label->screenCenter(), ll->screenCenter());
            if (d2 < threshold2) {
                return true;
            }
        }
    }
    return false;
}

void LabelManager::updateLabelSet(const ViewState& _viewState, float _dt, const Scene& _scene,
                            const std::vector<std::shared_ptr<Tile>>& _tiles,
                            const std::vector<std::unique_ptr<Marker>>& _markers,
                            TileManager& _tileManager) {

    m_transforms.clear();
    m_obbs.clear();

    /// Collect and update labels from visible tiles
    updateLabels(_viewState, _dt, _scene.styles(), _tiles, _markers, false);

    std::sort(m_labels.begin(), m_labels.end(), LabelManager::priorityComparator);

    /// Mark labels to skip transitions

    if (int(m_lastZoom) != int(_viewState.zoom)) {
        skipTransitions(_scene, _tiles, _tileManager, _viewState.zoom);
        m_lastZoom = _viewState.zoom;
    }

    m_isect2d.resize({_viewState.viewportSize.x / 256, _viewState.viewportSize.y / 256},
                     {_viewState.viewportSize.x, _viewState.viewportSize.y});

    handleOcclusions(_viewState);

    // Update label state
    for (auto& entry : m_labels) {
        m_needUpdate |= entry.label->evalState(_dt);
    }

    std::sort(m_labels.begin(), m_labels.end(), LabelManager::zOrderComparator);

    Label::AABB screenBounds{0, 0, _viewState.viewportSize.x, _viewState.viewportSize.y};

    // Update label meshes
    for (auto& entry : m_labels) {

        if (!entry.label->visibleState()) { continue; }

        ScreenTransform transform { m_transforms, entry.transformRange };

        for (auto& obb : OBBBuffer{ m_obbs, entry.obbsRange }) {

            if (obb.getExtent().intersect(screenBounds)) {
                entry.label->addVerticesToMesh(transform, _viewState.viewportSize);
                break;
            }
        }
    }
}

void LabelManager::drawDebug(RenderState& rs, const View& _view) {

    if (!Tangram::getDebugFlag(Tangram::DebugFlags::labels)) {
        return;
    }

    for (auto& entry : m_labels) {
        auto* label = entry.label;

        if (label->type() == Label::Type::debug) { continue; }

        glm::vec2 sp = label->screenCenter();

        // draw bounding box
        switch (label->state()) {
        case Label::State::sleep:
            Primitives::setColor(rs, 0x0000ff);
            break;
        case Label::State::visible:
            Primitives::setColor(rs, 0x000000);
            break;
        case Label::State::none:
            Primitives::setColor(rs, 0x0000ff);
            break;
        case Label::State::dead:
            Primitives::setColor(rs, 0xff00ff);
            break;
        case Label::State::fading_in:
            Primitives::setColor(rs, 0xffff00);
            break;
        case Label::State::fading_out:
            Primitives::setColor(rs, 0xff0000);
            break;
        default:
            Primitives::setColor(rs, 0x999999);
        }

#if DEBUG_OCCLUSION
        if (label->isOccluded()) {
            Primitives::setColor(rs, 0xff0000);
            if (label->occludedLastFrame()) {
                Primitives::setColor(rs, 0xffff00);
            }
        } else if (label->occludedLastFrame()) {
            Primitives::setColor(rs, 0x00ff00);
        } else {
            Primitives::setColor(rs, 0x000000);
        }
#endif

        for (auto& obb : OBBBuffer{ m_obbs, entry.obbsRange }) {
            Primitives::drawPoly(rs, &(obb.getQuad())[0], 4);
        }

        if (label->relative() && label->relative()->visibleState() && !label->relative()->isOccluded()) {
            Primitives::setColor(rs, 0xff0000);
            Primitives::drawLine(rs, m_obbs[entry.obbsRange.start].getCentroid(),
                                 label->relative()->screenCenter());
        }

        if (label->type() == Label::Type::curved) {
            //for (int i = entry.transform.start; i < entry.transform.end()-2; i++) {
            for (int i = entry.transformRange.start; i < entry.transformRange.end()-1; i++) {
                if (i % 2 == 0) {
                    Primitives::setColor(rs, 0xff0000);
                } else {
                    Primitives::setColor(rs, 0x0000ff);

                }
                Primitives::drawLine(rs, glm::vec2(m_transforms.points[i]),
                                     glm::vec2(m_transforms.points[i+1]));
            }
        }
#if 0
        // draw offset
        glm::vec2 rot = label->screenTransform().rotation;
        glm::vec2 offset = label->options().offset;
        if (label->relative()) { offset += label->relative()->options().offset; }
        offset = rotateBy(offset, rot);

        Primitives::setColor(rs, 0x000000);
        Primitives::drawLine(rs, sp, sp - glm::vec2(offset.x, -offset.y));
#endif

        // draw projected anchor point
        Primitives::setColor(rs, 0x0000ff);
        Primitives::drawRect(rs, sp - glm::vec2(1.f), sp + glm::vec2(1.f));

#if 0
        if (label->options().repeatGroup != 0 && label->state() == Label::State::visible) {
            size_t seed = 0;
            hash_combine(seed, label->options().repeatGroup);
            float repeatDistance = label->options().repeatDistance;

            Primitives::setColor(rs, seed);
            Primitives::drawLine(rs, label->screenCenter(),
                                 glm::vec2(repeatDistance, 0.f) + label->screenCenter());

            float off = M_PI / 6.f;
            for (float pad = 0.f; pad < M_PI * 2.f; pad += off) {
                glm::vec2 p0 = glm::vec2(cos(pad), sin(pad)) * repeatDistance
                    + label->screenCenter();
                glm::vec2 p1 = glm::vec2(cos(pad + off), sin(pad + off)) * repeatDistance
                    + label->screenCenter();
                Primitives::drawLine(rs, p0, p1);
            }
        }
#endif
    }

    glm::vec2 split(_view.getWidth() / 256, _view.getHeight() / 256);
    glm::vec2 res(_view.getWidth(), _view.getHeight());
    const short xpad = short(ceilf(res.x / split.x));
    const short ypad = short(ceilf(res.y / split.y));

    Primitives::setColor(rs, 0x7ef586);
    short x = 0, y = 0;
    for (int j = 0; j < split.y; ++j) {
        for (int i = 0; i < split.x; ++i) {
            AABB cell(x, y, x + xpad, y + ypad);
            Primitives::drawRect(rs, {x, y}, {x + xpad, y + ypad});
            x += xpad;
            if (x >= res.x) {
                x = 0;
                y += ypad;
            }
        }
    }
}

}
