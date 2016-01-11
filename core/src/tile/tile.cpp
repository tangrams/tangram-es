#include "tile.h"

#include "data/dataSource.h"
#include "scene/scene.h"
#include "scene/dataLayer.h"
#include "scene/styleContext.h"
#include "scene/drawRule.h"
#include "style/style.h"
#include "view/view.h"
#include "tile/tileID.h"
#include "labels/labelMesh.h"
#include "gl/vboMesh.h"
#include "gl/shaderProgram.h"

#include "glm/gtc/matrix_transform.hpp"

#include <algorithm>

namespace Tangram {

Tile::Tile(TileID _id, const MapProjection& _projection, const DataSource* _source) :
    m_id(_id),
    m_projection(&_projection),
    m_visible(false),
    m_priority(0),
    m_sourceId(_source ? _source->id() : 0),
    m_sourceGeneration(_source ? _source->generation() : 0) {

    BoundingBox bounds(_projection.TileBounds(_id));

    m_scale = bounds.width();
    m_inverseScale = 1.0/m_scale;

    updateTileOrigin(_id.wrap);

    // Init model matrix to size of tile
    m_modelMatrix = glm::scale(glm::mat4(1.0), glm::vec3(m_scale));
}

Tile::~Tile() {

}

//Note: This could set tile origin to be something different than the one if TileID's wrap is used.
// But, this is required for wrapped tiles which are picked up from the cache
void Tile::updateTileOrigin(const int _wrap) {
    BoundingBox bounds(m_projection->TileBounds(m_id));

    m_tileOrigin = { bounds.min.x, bounds.max.y }; // South-West corner
    // negative y coordinate: to change from y down to y up (tile system has y down and gl context we use has y up).
    m_tileOrigin.y *= -1.0;

    auto mapBound = m_projection->MapBounds();
    auto mapSpan = mapBound.max.x - mapBound.min.x;

    m_tileOrigin.x += (mapSpan * _wrap);
}

void Tile::initGeometry(uint32_t _size) {
    m_geometry.resize(_size);
}

void Tile::build(StyleContext& _ctx, const Scene& _scene, const TileData& _data,
                 const DataSource& _source) {

    // Initialize m_geometry
    initGeometry(_scene.styles().size());

    const auto& layers = _scene.layers();

    _ctx.setGlobalZoom(m_id.z);

    for (auto& style : _scene.styles()) {
        style->onBeginBuildTile(*this);
    }

    DrawRuleMergeSet ruleSet;

    for (const auto& datalayer : layers) {

        if (datalayer.source() != _source.name()) { continue; }

        for (const auto& collection : _data.layers) {

            if (!collection.name.empty()) {
                const auto& dlc = datalayer.collections();
                bool layerContainsCollection =
                    std::find(dlc.begin(), dlc.end(), collection.name) != dlc.end();

                if (!layerContainsCollection) { continue; }
            }

            for (const auto& feat : collection.features) {
                ruleSet.apply(feat, _scene, datalayer, _ctx, *this);
            }
        }
    }

    for (auto& style : _scene.styles()) {
        style->onEndBuildTile(*this);
    }

    for (auto& geometry : m_geometry) {
        if (geometry) {
            geometry->compileVertexBuffer();
        }
    }
}

void Tile::update(float _dt, const View& _view) {

    // Apply tile-view translation to the model matrix
    const auto& viewOrigin = _view.getPosition();
    m_modelMatrix[3][0] = m_tileOrigin.x - viewOrigin.x;
    m_modelMatrix[3][1] = m_tileOrigin.y - viewOrigin.y;

}

void Tile::reset() {
    for (auto& entry : m_geometry) {
        if (!entry) { continue; }
        auto labelMesh = dynamic_cast<LabelMesh*>(entry.get());
        if (!labelMesh) { continue; }
        labelMesh->reset();
    }
}

void Tile::draw(const Style& _style, const View& _view) {

    auto& styleMesh = getMesh(_style);

    if (styleMesh) {
        auto& shader = _style.getShaderProgram();

        float zoomAndProxy = m_proxyCounter > 0 ? -m_id.z : m_id.z;

        shader->setUniformMatrix4f("u_model", m_modelMatrix);
        shader->setUniformf("u_tile_origin", m_tileOrigin.x, m_tileOrigin.y, m_id.z);

        styleMesh->draw(*shader);
    }
}

std::unique_ptr<VboMesh>& Tile::getMesh(const Style& _style) {
    static std::unique_ptr<VboMesh> NONE = nullptr;

    if (_style.getID() >= m_geometry.size()) { return NONE; }

    return m_geometry[_style.getID()];
}

size_t Tile::getMemoryUsage() const {
    if (m_memoryUsage == 0) {
        for (auto& entry : m_geometry) {
            if (entry) {
                m_memoryUsage += entry->bufferSize();
            }
        }
    }

    return m_memoryUsage;
}

}
