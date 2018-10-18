#include "tile/tile.h"

#include "data/tileSource.h"
#include "labels/labelSet.h"
#include "style/style.h"
#include "tile/tileID.h"
#include "util/mapProjection.h"
#include "view/view.h"

#include "glm/gtc/matrix_transform.hpp"

namespace Tangram {

Tile::Tile(TileID _id, const TileSource* _source) :
    m_id(_id),
    m_sourceId(_source ? _source->id() : 0),
    m_sourceGeneration(_source ? _source->generation() : 0) {

    m_scale = MapProjection::metersPerTileAtZoom(_id.z);
    m_tileOrigin = MapProjection::tileSouthWestCorner(_id);

    // Init model matrix to size of tile
    m_modelMatrix = glm::scale(glm::mat4(1.0), glm::vec3(m_scale));
}


LngLat Tile::coordToLngLat(const glm::vec2& _tileCoord) const {
    glm::dvec2 meters = glm::dvec2(_tileCoord) * m_scale + m_tileOrigin;
    LngLat degrees = MapProjection::projectedMetersToLngLat(meters);
    return degrees;
}

Tile::~Tile() {}

void Tile::initGeometry(uint32_t _size) {
    m_geometry.resize(_size);
}

void Tile::update(float _dt, const View& _view) {
    // Get the relative position of the *center* of the tile, to ensure that the result places the tile as close to
    // the view center as possible.
    auto centerOffset = glm::dvec2(m_scale / 2.0);
    auto centerRelativeMeters = _view.getRelativeMeters(m_tileOrigin + centerOffset);
    auto originRelativeMeters = centerRelativeMeters - centerOffset;

    // Apply tile-view translation to the model matrix
    m_modelMatrix[3][0] = static_cast<float>(originRelativeMeters.x);
    m_modelMatrix[3][1] = static_cast<float>(originRelativeMeters.y);

    m_mvp = _view.getViewProjectionMatrix() * m_modelMatrix;
}

void Tile::resetState() {
    for (auto& entry : m_geometry) {
        if (!entry) { continue; }
        auto labelSet = dynamic_cast<LabelSet*>(entry.get());
        if (!labelSet) { continue; }
        labelSet->reset();
    }
}

void Tile::setMesh(const Style& _style, std::unique_ptr<StyledMesh> _mesh) {
    size_t id = _style.getID();
    if (id >= m_geometry.size()) {
        m_geometry.resize(id+1);
    }
    m_geometry[_style.getID()] = std::move(_mesh);
}

const std::unique_ptr<StyledMesh>& Tile::getMesh(const Style& _style) const {
    static std::unique_ptr<StyledMesh> NONE = nullptr;
    if (_style.getID() >= m_geometry.size()) { return NONE; }

    return m_geometry[_style.getID()];
}

void Tile::setSelectionFeatures(const fastmap<uint32_t, std::shared_ptr<Properties>> _selectionFeatures) {
    m_selectionFeatures = _selectionFeatures;
}

std::shared_ptr<Properties> Tile::getSelectionFeature(uint32_t _id) const {
    auto it = m_selectionFeatures.find(_id);
    if (it != m_selectionFeatures.end()) {
        return it->second;
    }
    return nullptr;
}

size_t Tile::getMemoryUsage() const {
    if (m_memoryUsage == 0) {
        for (auto& entry : m_geometry) {
            if (entry) {
                m_memoryUsage += entry->bufferSize();
            }
        }
        for (auto& raster : m_rasters) {
            if (raster.texture) {
                m_memoryUsage += raster.texture->bufferSize();
            }
        }
    }

    return m_memoryUsage;
}

}
