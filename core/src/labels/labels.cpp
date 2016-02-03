#include "labels.h"

#include "tangram.h"
#include "platform.h"
#include "gl/shaderProgram.h"
#include "gl/primitives.h"
#include "view/view.h"
#include "style/style.h"
#include "style/pointStyle.h"
#include "tile/tile.h"
#include "tile/tileCache.h"
#include "labels/labelMesh.h"
// TODO: remove (for AlfonsLabel)
#include "style/alfonsStyle.h"

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

            auto labelMesh = dynamic_cast<const LabelSet*>(mesh.get());
            if (!labelMesh) { continue; }

            for (auto& label : labelMesh->getLabels()) {
                if (!label->update(mvp, screenSize, _dz)) {
                    // skip dead labels
                    continue;
                }

                label->setProxy(proxyTile);
                m_labels.push_back(label.get());
            }
        }
    }
}

void skip(const std::vector<const Style*>& _styles, Tile& _tile, Tile& _proxy) {

    for (const auto& style : _styles) {

        auto* mesh0 = dynamic_cast<const LabelSet*>(_tile.getMesh(*style).get());
        if (!mesh0) { continue; }

        auto* mesh1 = dynamic_cast<const LabelSet*>(_proxy.getMesh(*style).get());
        if (!mesh1) { continue; }

        for (auto& l0 : mesh0->getLabels()) {
            if (!l0->canOcclude()) { continue; }
            if ((l0->state() & Label::State::wait_occ) == 0) { continue; }

            for (auto& l1 : mesh1->getLabels()) {
                //if (!l1->canOcclude() || l0->hash() != l1->hash()) { continue; }
                if (!l1->canOcclude() || l0->options().repeatGroup != l1->options().repeatGroup) {
                    continue;
                }

                float d2 = glm::distance2(l0->transform().state.screenPos,
                                          l1->transform().state.screenPos);

                // The new label lies within the circle defined by the bbox of l0
                if (sqrt(d2) < std::max(l0->dimension().x, l0->dimension().y)) {

                    auto* t = dynamic_cast<const AlfonsLabel*>(l0.get());
                    if (t && t->options().properties) {
                        auto v = t->options().properties->get("name");
                        if (v.is<std::string>())
                            LOG("skip transition: %s", v.get<std::string>().c_str());
                    }
                    // LOG("skip transition... ");

                    l0->skipTransitions();
                }
            }
        }
    }
}

void Labels::skipTransitions(const std::vector<std::unique_ptr<Style>>& _styles,
                             const std::vector<std::shared_ptr<Tile>>& _tiles,
                             std::unique_ptr<TileCache>& _cache, float _currentZoom) const
{
    std::vector<const Style*> styles;

    for (const auto& style : _styles) {
        if (dynamic_cast<const AlfonsStyle*>(style.get()) ||
            dynamic_cast<const PointStyle*>(style.get())) {
            styles.push_back(style.get());
        }
    }

    for (const auto& tile : _tiles) {
        TileID tileID = tile->getID();
        std::shared_ptr<Tile> proxy;

        if (m_lastZoom < _currentZoom) {
            // zooming in, add the one cached parent tile
            proxy = _cache->contains(tile->sourceID(), tileID.getParent());
            if (proxy) { skip(styles, *tile, *proxy); }
        } else {
            // zooming out, add the 4 cached children tiles
            proxy = _cache->contains(tile->sourceID(), tileID.getChild(0));
            if (proxy) { skip(styles, *tile, *proxy); }

            proxy = _cache->contains(tile->sourceID(), tileID.getChild(1));
            if (proxy) { skip(styles, *tile, *proxy); }

            proxy = _cache->contains(tile->sourceID(), tileID.getChild(2));
            if (proxy) { skip(styles, *tile, *proxy); }

            proxy = _cache->contains(tile->sourceID(), tileID.getChild(3));
            if (proxy) { skip(styles, *tile, *proxy); }
        }
    }
}

void Labels::checkRepeatGroups(std::vector<Label*>& _visibleSet) const {
    struct GroupElement {
        glm::vec2 position;

        bool operator==(const GroupElement& _ge) {
            return _ge.position == position;
        };
    };

    std::map<size_t, std::vector<GroupElement>> repeatGroups;

    for (Label* textLabel : _visibleSet) {
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
                textLabel->occlude();
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

// TODO when view and tiles dont have changed just update the alpha transitions!

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

    updateLabels(_styles, _tiles, _dt, dz, _view);


    /// Mark labels to skip transitions

    if (int(m_lastZoom) != int(_view.getZoom())) {
        skipTransitions(_styles, _tiles, _cache, currentZoom);
    }

    /// Manage occlusions

    // Update collision context size
    m_isect2d.resize({_view.getWidth() / 256, _view.getHeight() / 256},
                     {_view.getWidth(), _view.getHeight()});

    // Broad phase collision detection
    for (auto* label : m_labels) {
        if (!label->isOccluded() && label->canOcclude()) {
            m_aabbs.push_back(label->aabb());
            m_aabbs.back().m_userData = (void*)label;
        }
    }
    m_isect2d.intersect(m_aabbs);

    // Narrow Phase, resolve conflicts
    for (auto& pair : m_isect2d.pairs) {
        const auto& aabb1 = m_aabbs[pair.first];
        const auto& aabb2 = m_aabbs[pair.second];

        auto l1 = static_cast<Label*>(aabb1.m_userData);
        auto l2 = static_cast<Label*>(aabb2.m_userData);

        if (l1->isOccluded() || l2->isOccluded()) {
            // One of this pair is already occluded.
            // => conflict solved
            continue;
        }

        if (!intersect(l1->obb(), l2->obb())) { continue; }

        if (l1->isProxy() != l2->isProxy()) {
            // check first is the label belongs to a proxy tile
            if (l1->isProxy()) {
                l1->occlude();
            } else {
                l2->occlude();
            }
        } else if (l1->options().priority != l2->options().priority) {
            // lower numeric priority means higher priority
            if(l1->options().priority > l2->options().priority) {
                l1->occlude();
            } else {
                l2->occlude();
            }
        } else if (l1->occludedLastFrame() != l2->occludedLastFrame()) {
            // keep the one that way active previously
            if (l1->occludedLastFrame()) {
                l1->occlude();
            } else {
                l2->occlude();
            }
        } else if (l1->visibleState() != l2->visibleState()) {
            // keep the visible one, different from occludedLastframe
            // when one lets labels fade out
            if (!l1->visibleState()) {
                l1->occlude();
            } else {
                l2->occlude();
            }
        } else {
            // just so it is consistent between two instances
            if (l1 < l2) {
                l1->occlude();
            } else {
                l2->occlude();
            }
        }
    }

    /// Apply repeat groups

    std::vector<Label*> repeatGroupSet;
    for (auto* label : m_labels) {
        if (!label->canOcclude() || label->isOccluded()) {
            continue;
        }

        if (label->options().repeatDistance == 0.f) {
            continue;
        }
        if (label->type() != Label::Type::line) { continue; }
        repeatGroupSet.push_back(label);
    }

    // Ensure the labels are always treated in the same order in the visible set
    std::sort(repeatGroupSet.begin(), repeatGroupSet.end(), [](auto _a, auto _b) {
        return glm::length2(_a->transform().modelPosition1) < glm::length2(_b->transform().modelPosition1);
    });

    checkRepeatGroups(repeatGroupSet);


    /// Update label meshes

    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());

    m_needUpdate = false;
    for (auto* label : m_labels) {
        m_needUpdate |= label->evalState(screenSize, _dt);
        label->pushTransform();
    }

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
            switch (label->state()) {
                case Label::State::sleep:
                    Primitives::setColor(0x00ff00);
                    break;
                case Label::State::visible:
                    Primitives::setColor(0x000000);
                    break;
                case Label::State::wait_occ:
                    Primitives::setColor(0xff00ff);
                    break;
                case Label::State::fading_in:
                    Primitives::setColor(0xffff00);
                    break;
                case Label::State::fading_out:
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

            continue;

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
