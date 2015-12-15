#include "labels.h"

#include "tangram.h"
#include "platform.h"
#include "gl/shaderProgram.h"
#include "gl/primitives.h"
#include "view/view.h"
#include "style/material.h"
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

void Labels::updateLabels(const std::vector<std::unique_ptr<Style>>& _styles,
                          const std::vector<std::shared_ptr<Tile>>& _tiles,
                          float _dt, float _dz, const View& _view)
{
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

            const LabelMesh* labelMesh = dynamic_cast<const LabelMesh*>(mesh.get());
            if (!labelMesh) { continue; }

            for (auto& label : labelMesh->getLabels()) {
                m_needUpdate |= label->update(mvp, screenSize, _dt, _dz);

                label->setProxy(proxyTile);

                if (label->canOcclude()) {
                    m_aabbs.push_back(label->aabb());
                    m_aabbs.back().m_userData = (void*)label.get();
                }
                m_labels.push_back(label.get());
            }
        }
    }
}

void Labels::solveOcclusions() {
    // Broad phase
    m_isect2d.intersect(m_aabbs);

    // Narrow phase
    std::set<std::pair<Label*, Label*>> occlusions;

    for (auto pair : m_isect2d.pairs) {
        const auto& aabb1 = m_aabbs[pair.first];
        const auto& aabb2 = m_aabbs[pair.second];

        auto l1 = static_cast<Label*>(aabb1.m_userData);
        auto l2 = static_cast<Label*>(aabb2.m_userData);

        if (intersect(l1->obb(), l2->obb())) {
            occlusions.insert({l1, l2});
        }
    }

    // Manage priorities
    for (auto& pair : occlusions) {
        if (!pair.first->occludedLastFrame() || !pair.second->occludedLastFrame()) {
            // check first is the label belongs to a proxy tile
            if (pair.first->isProxy() && !pair.second->isProxy()) {
                pair.first->setOcclusion(true);
            } else if (!pair.first->isProxy() && pair.second->isProxy()) {
                pair.second->setOcclusion(true);
            } else {
                // lower numeric priority means higher priority
                if (pair.first->options().priority < pair.second->options().priority) {
                    pair.second->setOcclusion(true);
                } else {
                    pair.first->setOcclusion(true);
                }
            }
        }
    }
}


void Labels::skipTransitions(const std::vector<std::unique_ptr<Style>>& _styles,
                             const std::vector<std::shared_ptr<Tile>>& _tiles,
                             std::unique_ptr<TileCache>& _cache, float _currentZoom)
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
                const LabelMesh* mesh0 = dynamic_cast<const LabelMesh*>(m0.get());
                if (!mesh0) { continue; }
                const auto& m1 = t1->getMesh(*style);
                if (!m1) { continue; }
                const LabelMesh* mesh1 = static_cast<const LabelMesh*>(m1.get());

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

void Labels::checkRepeatGroups() {
    auto textLabelIt = m_visibleTextSet.begin();
    while (textLabelIt != m_visibleTextSet.end()) {
        auto textLabel = *textLabelIt;
        CollideComponent component;
        component.position = textLabel->transform().state.screenPos;
        component.userData = (void*)textLabel;

        std::size_t seed = 0;
        hash_combine(seed, textLabel->text);

        component.group = seed;
        component.mask = seed;

        auto it = m_repeatGroups.find(seed);
        if (it != m_repeatGroups.end()) {
            std::vector<CollideComponent>& group = m_repeatGroups[seed];

            if (std::find(group.begin(), group.end(), component) == group.end()) {
                std::vector<CollideComponent> newGroup(group);
                newGroup.push_back(component);

                isect2d::CollideOption options;
                options.thresholdDistance = 100.0f;
                options.rule = isect2d::CollideRuleOption::UNIDIRECTIONNAL;

                auto collisionMaskPairs = isect2d::intersect(newGroup, options);

                if (collisionMaskPairs.size() == 0) {
                    group.push_back(component);
                    textLabelIt++;
                } else {
                    textLabel->setOcclusion(true);
                    m_visibleTextSet.erase(textLabelIt);
                }
            } else {
                textLabelIt++;
            }
        } else {
            m_repeatGroups[seed].push_back(component);
            textLabelIt++;
        }
    }
}

void Labels::update(const View& _view, float _dt,
                    const std::vector<std::unique_ptr<Style>>& _styles,
                    const std::vector<std::shared_ptr<Tile>>& _tiles,
                    std::unique_ptr<TileCache>& _cache)
{
    m_needUpdate = false;

    // Could clear this at end of function unless debug draw is active
    m_labels.clear();
    m_aabbs.clear();
    m_visibleTextSet.clear();
    m_collideComponents.clear();
    m_repeatGroups.clear();

    float currentZoom = _view.getZoom();
    float dz = currentZoom - std::floor(currentZoom);

    /// Collect and update labels from visible tiles

    updateLabels(_styles, _tiles, _dt, dz, _view);

    // Update collision context size

    m_isect2d.resize({_view.getWidth() / 256, _view.getHeight() / 256},
                     {_view.getWidth(), _view.getHeight()});

    /// Manage occlusions

    solveOcclusions();

    /// Mark labels to skip transitions

    if ((int) m_lastZoom != (int) _view.getZoom()) {
        skipTransitions(_styles, _tiles, _cache, currentZoom);
    }

    /// Update label meshes

    for (auto label : m_labels) {
        label->occlusionSolved();
        label->pushTransform();

        if (label->canOcclude()) {
            if (!label->visibleState()) { continue; }
            TextLabel* textLabel = dynamic_cast<TextLabel*>(label);
            if (!textLabel) { continue; }
            m_visibleTextSet.push_back(textLabel);
        }
    }

    // Ensure the labels are always treated in the same order in the visible set
    std::sort(m_visibleTextSet.begin(), m_visibleTextSet.end(), [](TextLabel* _a, TextLabel* _b) {
        return glm::length2(_a->transform().modelPosition1) < glm::length2(_b->transform().modelPosition1);
    });

    /// Apply repeat groups

    checkRepeatGroups();

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

            const LabelMesh* labelMesh = dynamic_cast<const LabelMesh*>(mesh.get());
            if (!labelMesh) { continue; }

            for (auto& label : labelMesh->getLabels()) {

                auto& options = label->options();
                if (!options.interactive) { continue; }

                if (!_visibleOnly) {
                    label->updateScreenTransform(mvp, screenSize, false);
                    label->updateBBoxes(dz);
                }

                if (isect2d::intersect(label->obb(), obb)) {
                    float distance = glm::length2(label->transform().state.screenPos - touchPoint);

                    m_touchItems.push_back({options.properties, std::sqrt(distance)});
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
