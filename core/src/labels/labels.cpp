#include "labels.h"
#include "tangram.h"
#include "tile/tile.h"
#include "gl/primitives.h"
#include "view/view.h"
#include "style/style.h"

#include "glm/gtc/matrix_transform.hpp"

namespace Tangram {

Labels::Labels() {}

Labels::~Labels() {}

int Labels::LODDiscardFunc(float _maxZoom, float _zoom) {
    return (int) MIN(floor(((log(-_zoom + (_maxZoom + 2)) / log(_maxZoom + 2) * (_maxZoom )) * 0.5)), MAX_LOD);
}


void Labels::update(const View& _view, float _dt, const std::vector<std::unique_ptr<Style>>& _styles,
                    const std::map<TileID, std::shared_ptr<Tile>>& _tiles) {

    m_needUpdate = false;

    // float zoom = _view.getZoom();
    // int lodDiscard = LODDiscardFunc(View::s_maxZoom, zoom);
    // logMsg("loddiscard %f %d\n", zoom, lodDiscard);

    std::set<std::pair<Label*, Label*>> occlusions;

    // Could clear this at end of function unless debug draw is active
    m_labels.clear();
    m_aabbs.clear();

    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());

    //// Collect labels from visible tiles

    for (const auto& mapIDandTile : _tiles) {
        const auto& tile = mapIDandTile.second;

        if (!tile->isReady()) { continue; }

        // discard based on level of detail
        // if ((zoom - tile->getID().z) > lodDiscard) {
        //     logMsg("discard %d %d %d\n", tile->getID().z, tile->getID().x, tile->getID().y);
        //     continue;
        // }

        glm::mat4 mvp = _view.getViewProjectionMatrix() * tile->getModelMatrix();

        for (const auto& style : _styles) {
            VboMesh* mesh = tile->getMesh(*style);
            if (!mesh) { continue; }

            auto labelMesh = dynamic_cast<LabelMesh*>(mesh);
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
    auto pairs = intersect(m_aabbs, {4, 4}, {_view.getWidth(), _view.getHeight()});

    for (auto pair : pairs) {
        const auto& aabb1 = m_aabbs[pair.first];
        const auto& aabb2 = m_aabbs[pair.second];

        auto l1 = (Label*)aabb1.m_userData;
        auto l2 = (Label*)aabb2.m_userData;

        // narrow phase
        if (intersect(l1->getOBB(), l2->getOBB())) { occlusions.insert({l1, l2}); }
    }

    // no priorities, only occlude one of the two occluded label
    for (auto& pair : occlusions) {
        if (!pair.first->occludedLastFrame()) {
            if (pair.second->getState() == Label::State::wait_occ) {
                pair.second->setOcclusion(true);
            }
        }
        if (!pair.second->occludedLastFrame()) {
            pair.first->setOcclusion(true);
        }
    }

    //// Update label meshes

    for (auto label : m_labels) {
        label->occlusionSolved();
        label->pushTransform();
    }

    // Request for render if labels are in fading in/out states
    if (m_needUpdate)
        requestRender();
}

void Labels::drawDebug(const View& _view) {

    if (!Tangram::getDebugFlag(Tangram::DebugFlags::labels)) {
        return;
    }

    for (auto label : m_labels) {
        if (label->canOcclude()) {
            Primitives::drawPoly(reinterpret_cast<const glm::vec2*>(label->getOBB().getQuad()),
                                 4, { _view.getWidth(), _view.getHeight() });
        }
    }

    glm::vec2 split(4, 4);
    glm::vec2 res(_view.getWidth(), _view.getHeight());
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

}
