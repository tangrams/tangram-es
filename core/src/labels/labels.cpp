#include "labels.h"
#include "tangram.h"
#include "tile/tile.h"
#include "gl/primitives.h"
#include "view/view.h"
#include "style/style.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/norm.hpp"

namespace Tangram {

Labels::Labels() : m_needUpdate(false) {}

Labels::~Labels() {}

int Labels::LODDiscardFunc(float _maxZoom, float _zoom) {
    return (int) MIN(floor(((log(-_zoom + (_maxZoom + 2)) / log(_maxZoom + 2) * (_maxZoom )) * 0.5)), MAX_LOD);
}


void Labels::update(const View& _view, float _dt, const std::vector<std::unique_ptr<Style>>& _styles,
                    const std::vector<std::shared_ptr<Tile>>& _tiles) {

    m_needUpdate = false;

    // float zoom = _view.getZoom();
    // int lodDiscard = LODDiscardFunc(View::s_maxZoom, zoom);
    // LOG("loddiscard %f %d", zoom, lodDiscard);

    std::set<std::pair<Label*, Label*>> occlusions;

    // Could clear this at end of function unless debug draw is active
    m_labels.clear();
    m_aabbs.clear();

    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());

    //// Collect labels from visible tiles

    for (const auto& tile : _tiles) {

        // discard based on level of detail
        // if ((zoom - tile->getID().z) > lodDiscard) {
        //     LOG("discard %d %d %d", tile->getID().z, tile->getID().x, tile->getID().y);
        //     continue;
        // }

        glm::mat4 mvp = _view.getViewProjectionMatrix() * tile->getModelMatrix();

        for (const auto& style : _styles) {
            const auto& mesh = tile->getMesh(*style);
            if (!mesh) { continue; }

            const LabelMesh* labelMesh = dynamic_cast<const LabelMesh*>(mesh.get());
            if (!labelMesh) { continue; }

            for (auto& label : labelMesh->getLabels()) {
                m_needUpdate |= label->update(mvp, screenSize, _dt);

                if (label->canOcclude()) {
                    m_aabbs.push_back(label->getAABB());
                    m_aabbs.back().m_userData = (void*)label.get();
                }
                m_labels.push_back(label.get());
            }
        }
    }

    //// Manage occlusions

    // broad phase
    m_isect2d.resize({_view.getWidth() / 256, _view.getHeight() / 256}, {_view.getWidth(), _view.getHeight()});
    m_isect2d.intersect(m_aabbs);

    // narrow phase
    for (auto pair : m_isect2d.pairs) {
        const auto& aabb1 = m_aabbs[pair.first];
        const auto& aabb2 = m_aabbs[pair.second];

        auto l1 = static_cast<Label*>(aabb1.m_userData);
        auto l2 = static_cast<Label*>(aabb2.m_userData);

        if (intersect(l1->getOBB(), l2->getOBB())) { occlusions.insert({l1, l2}); }
    }

    for (auto& pair : occlusions) {
        if (!pair.first->occludedLastFrame() || !pair.second->occludedLastFrame()) {
            // lower numeric priority means higher priority
            if (pair.first->getOptions().priority < pair.second->getOptions().priority) {
                pair.second->setOcclusion(true);
            } else {
                pair.first->setOcclusion(true);
            }
        }
    }

    //// Update label meshes

    for (auto label : m_labels) {
        label->occlusionSolved();
        label->pushTransform();
    }

    // Request for render if labels are in fading in/out states
    if (m_needUpdate) {
        requestRender();
    }
}

const std::vector<std::shared_ptr<Properties>>& Labels::getFeaturesAtPoint(const View& _view, float _dt,
                                                                           const std::vector<std::unique_ptr<Style>>& _styles,
                                                                           const std::vector<std::shared_ptr<Tile>>& _tiles,
                                                                           float _x, float _y, bool _visibleOnly) {
    // FIXME dpi dependent threshold
    const float thumbSize = 50;

    m_touchItems.clear();

    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());
    glm::vec2 touchPoint(_x, _y);

    OBB obb(_x - thumbSize/2, _y - thumbSize/2, 0, thumbSize, thumbSize);

    std::shared_ptr<Properties> selectedItem;
    float minDistance = std::numeric_limits<float>::max();

    for (const auto& tile : _tiles) {

        glm::mat4 mvp = _view.getViewProjectionMatrix() * tile->getModelMatrix();

        for (const auto& style : _styles) {
            const auto& mesh = tile->getMesh(*style);
            if (!mesh) { continue; }

            const LabelMesh* labelMesh = dynamic_cast<const LabelMesh*>(mesh.get());
            if (!labelMesh) { continue; }

            for (auto& label : labelMesh->getLabels()) {

                auto& options = label->getOptions();
                if (!options.interactive) { continue; }

                if (!_visibleOnly) {
                    label->updateScreenTransform(mvp, screenSize, false);
                    label->updateBBoxes();
                }

                if (isect2d::intersect(label->getOBB(), obb)) {
                    float distance = glm::length2(label->getTransform().state.screenPos - touchPoint);

                    if (distance < minDistance) {
                        minDistance = distance;
                        selectedItem = options.properties;
                    }
                }
            }
        }
    }

    if (selectedItem) {
        m_touchItems.push_back(selectedItem);
    }

    return m_touchItems;
}



void Labels::drawDebug(const View& _view) {

    if (!Tangram::getDebugFlag(Tangram::DebugFlags::labels)) {
        return;
    }

    for (auto label : m_labels) {
        if (label->canOcclude()) {
            glm::vec2 offset = label->getOptions().offset;
            glm::vec2 sp = label->getTransform().state.screenPos;
            float angle = label->getTransform().state.rotation;
            offset = glm::rotate(offset, angle);

            // draw bounding box
            Label::State state = label->getState();
            switch (state) {
                case Label::State::sleep:
                    Primitives::setColor(0x00ff00);
                    break;
                case Label::State::visible:
                    Primitives::setColor(0xffffff);
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

            Primitives::drawPoly(&(label->getOBB().getQuad())[0], 4);

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
