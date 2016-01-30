#include "labels.h"

#include "tangram.h"
#include "platform.h"
#include "gl/shaderProgram.h"
#include "gl/primitives.h"
#include "view/view.h"
#include "style/style.h"
#include "tile/tile.h"
#include "tile/tileCache.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/norm.hpp"

namespace Tangram {

Labels::Labels()
    : m_needUpdate(false),
      m_lastZoom(0.0f)
{}

Labels::~Labels() {}

int Labels::LODDiscardFunc(float _maxZoom, float _zoom) {
    return (int) MIN(floor(((log(-_zoom + (_maxZoom + 2)) / log(_maxZoom + 2) * (_maxZoom )) * 0.5)), MAX_LOD);
}

bool Labels::updateLabels(const std::vector<std::unique_ptr<Style>>& _styles,
                          const std::vector<std::shared_ptr<Tile>>& _tiles,
                          float _dt, float _dz, const View& _view)
{
    bool animate = false;

    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());

    // int lodDiscard = LODDiscardFunc(View::s_maxZoom, _view.getZoom());

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
                animate |= label->update(mvp, screenSize, _dt, _dz);

                label->setProxy(proxyTile);

                if (label->canOcclude()) {
                    m_aabbs.push_back(label->aabb());
                    m_aabbs.back().m_userData = (void*)label.get();
                }
                m_labels.push_back(label.get());
            }
        }
    }

    return animate;
}

std::set<std::pair<Label*, Label*>> Labels::narrowPhase(const CollisionPairs& _pairs) const {
    std::set<std::pair<Label*, Label*>> occlusions;

    for (auto pair : _pairs) {
        const auto& aabb1 = m_aabbs[pair.first];
        const auto& aabb2 = m_aabbs[pair.second];

        auto l1 = static_cast<Label*>(aabb1.m_userData);
        auto l2 = static_cast<Label*>(aabb2.m_userData);

        if (intersect(l1->obb(), l2->obb())) {
            occlusions.insert({l1, l2});
        }
    }

    return occlusions;
}

void Labels::applyPriorities(const std::set<std::pair<Label*, Label*>> _occlusions) const {
    for (auto& pair : _occlusions) {
        if (!pair.first->occludedLastFrame() || !pair.second->occludedLastFrame()) {
            // check first is the label belongs to a proxy tile
            if (pair.first->isProxy() && !pair.second->isProxy()) {
                pair.first->occlude(Label::OcclusionType::collision);
            } else if (!pair.first->isProxy() && pair.second->isProxy()) {
                pair.second->occlude(Label::OcclusionType::collision);
            } else {
                // lower numeric priority means higher priority
                if (pair.first->options().priority < pair.second->options().priority) {
                    pair.second->occlude(Label::OcclusionType::collision);
                } else {
                    pair.first->occlude(Label::OcclusionType::collision);
                }
            }
        }
    }
}

void Labels::skipTransitions(const std::vector<std::unique_ptr<Style>>& _styles,
                             const std::vector<std::shared_ptr<Tile>>& _tiles,
                             std::unique_ptr<TileCache>& _cache, float _currentZoom) const
{
    for (const auto& t0 : _tiles) {
        TileID tileID = t0->getID();
        std::vector<std::shared_ptr<Tile>> tiles;

        if (m_lastZoom < _currentZoom) {
            // zooming in, add the one cached parent tile
            tiles.push_back(_cache->contains(t0->sourceID(), tileID.getParent()));
        } else {
            // zooming out, add the 4 cached children tiles
            tiles.push_back(_cache->contains(t0->sourceID(), tileID.getChild(0)));
            tiles.push_back(_cache->contains(t0->sourceID(), tileID.getChild(1)));
            tiles.push_back(_cache->contains(t0->sourceID(), tileID.getChild(2)));
            tiles.push_back(_cache->contains(t0->sourceID(), tileID.getChild(3)));
        }

        for (const auto& t1 : tiles) {
            if (!t1) { continue; }
            for (const auto& style : _styles) {
                const auto& m0 = t0->getMesh(*style);
                if (!m0) { continue; }
                auto mesh0 = dynamic_cast<const LabelSet*>(m0.get());
                if (!mesh0) { continue; }
                const auto& m1 = t1->getMesh(*style);
                if (!m1) { continue; }
                auto mesh1 = dynamic_cast<const LabelSet*>(m1.get());

                for (auto& l0 : mesh0->getLabels()) {
                    if (!l0->canOcclude()) { continue; }

                    for (auto& l1 : mesh1->getLabels()) {
                        if (!l1 || !l1->canOcclude() || l0->hash() != l1->hash()) {
                            continue;
                        }
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
    }
}

void Labels::checkRepeatGroups(std::vector<TextLabel*>& _visibleSet) const {
    struct GroupElement {
        glm::vec2 position;

        bool operator==(const GroupElement& _ge) {
            return _ge.position == position;
        };
    };

    std::map<size_t, std::vector<GroupElement>> repeatGroups;

    for (TextLabel* textLabel : _visibleSet) {
        auto& options = textLabel->options();
        GroupElement element { textLabel->center() };

        auto& group = repeatGroups[options.repeatGroup];
        if (group.empty()) {
            group.push_back(element);
            continue;
        }

        if (std::find(group.begin(), group.end(), element) != group.end()) {
            //Two tiles contain the same label - have the same screen position.
            continue;
        }

        float threshold2 = pow(options.repeatDistance, 2);

        bool add = true;
        for (const GroupElement& ge : group) {

            float d2 = distance2(ge.position, element.position);
            if (d2 < threshold2) {
                textLabel->occlude(Label::OcclusionType::repeat_group);
                add = false;
                break;
            }
        }

        if (add) {
            // No other label of this group within repeatDistance
            group.push_back(element);
        }
    }
}

void Labels::update(const View& _view, float _dt,
                    const std::vector<std::unique_ptr<Style>>& _styles,
                    const std::vector<std::shared_ptr<Tile>>& _tiles,
                    std::unique_ptr<TileCache>& _cache)
{
    // Could clear this at end of function unless debug draw is active
    m_labels.clear();
    m_aabbs.clear();

    float currentZoom = _view.getZoom();
    float dz = currentZoom - std::floor(currentZoom);

    /// Collect and update labels from visible tiles

    m_needUpdate = updateLabels(_styles, _tiles, _dt, dz, _view);

    /// Manage occlusions

    // Update collision context size

    m_isect2d.resize({_view.getWidth() / 256, _view.getHeight() / 256},
                     {_view.getWidth(), _view.getHeight()});

    // Broad phase collision detection
    m_isect2d.intersect(m_aabbs);

    // Narrow Phase
    auto occlusions = narrowPhase(m_isect2d.pairs);

    applyPriorities(occlusions);

    /// Mark labels to skip transitions

    if ((int) m_lastZoom != (int) _view.getZoom()) {
        skipTransitions(_styles, _tiles, _cache, currentZoom);
    }

    /// Update label meshes

    std::vector<TextLabel*> repeatGroupSet;

    for (auto label : m_labels) {
        label->occlusionSolved();
        label->pushTransform();

        if (label->canOcclude()) {
            if (!label->visibleState() && label->occlusionType() == Label::OcclusionType::collision) {
                continue;
            }
            if (label->options().repeatDistance == 0.f) {
                continue;
            }

            TextLabel* textLabel = dynamic_cast<TextLabel*>(label);
            if (!textLabel) { continue; }
            repeatGroupSet.push_back(textLabel);
        }
    }

    // Ensure the labels are always treated in the same order in the visible set
    std::sort(repeatGroupSet.begin(), repeatGroupSet.end(), [](TextLabel* _a, TextLabel* _b) {
        return glm::length2(_a->transform().modelPosition1) < glm::length2(_b->transform().modelPosition1);
    });

    /// Apply repeat groups

    checkRepeatGroups(repeatGroupSet);

    // Request for render if labels are in fading in/out states
    if (m_needUpdate) {
        requestRender();
    }

    m_lastZoom = currentZoom;
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

    for (auto label : m_labels) {
        if (label->canOcclude()) {
            glm::vec2 offset = label->options().offset;
            glm::vec2 sp = label->transform().state.screenPos;
            float angle = label->transform().state.rotation;
            offset = glm::rotate(offset, angle);

            // draw bounding box
            Label::State state = label->state();
            switch (state) {
                case Label::State::sleep:
                    Primitives::setColor(0x00ff00);
                    break;
                case Label::State::visible:
                    Primitives::setColor(0x000000);
                    break;
                case Label::State::wait_occ:
                    Primitives::setColor(0x0000ff);
                    break;
                case Label::State::fading_in:
                case Label::State::fading_out:
                    Primitives::setColor(0xffff00);
                    break;
                default:
                    Primitives::setColor(0xff0000);
            }

            Primitives::drawPoly(&(label->obb().getQuad())[0], 4);

            // draw offset
            Primitives::setColor(0x000000);
            Primitives::drawLine(sp, sp - offset);

            // draw projected anchor point
            Primitives::setColor(0x0000ff);
            Primitives::drawRect(sp - glm::vec2(1.f), sp + glm::vec2(1.f));

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
        }
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
