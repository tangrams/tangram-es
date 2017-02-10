#include "tile/tile.h"

#include "data/tileSource.h"
#include "labels/labelSet.h"
#include "style/style.h"
#include "tile/tileID.h"
#include "view/view.h"

#include "glm/gtc/matrix_transform.hpp"

namespace Tangram {

Tile::Tile(TileID _id, const MapProjection& _projection, const TileSource* _source) :
    m_id(_id),
    m_projection(&_projection),
    m_sourceId(_source ? _source->id() : 0),
    m_sourceGeneration(_source ? _source->generation() : 0) {

    BoundingBox bounds(_projection.TileBounds(_id));

    m_scale = bounds.width();
    m_inverseScale = 1.0/m_scale;

    updateTileOrigin(_id.wrap);

    // Init model matrix to size of tile
    m_modelMatrix = glm::scale(glm::mat4(1.0), glm::vec3(m_scale));
}


glm::dvec2 Tile::coordToLngLat(const glm::vec2& _tileCoord) const {
    double scale = 1.0 / m_inverseScale;

    glm::dvec2 meters = glm::dvec2(_tileCoord) * scale + m_tileOrigin;
    glm::dvec2 degrees = m_projection->MetersToLonLat(meters);

    return {degrees.x, degrees.y};
}

Tile::~Tile() {}

//Note: This could set tile origin to be something different than the one if TileID's wrap is used.
// But, this is required for wrapped tiles which are picked up from the cache
void Tile::updateTileOrigin(const int _wrap) {
    BoundingBox bounds(m_projection->TileBounds(m_id));

    m_tileOrigin = { bounds.min.x, bounds.max.y }; // South-West corner
    // negative y coordinate: to change from y down to y up
    // (tile system has y down and gl context we use has y up).
    m_tileOrigin.y *= -1.0;

    auto mapBound = m_projection->MapBounds();
    auto mapSpan = mapBound.max.x - mapBound.min.x;

    m_tileOrigin.x += (mapSpan * _wrap);
}

void Tile::initGeometry(uint32_t _size) {
    m_geometry.resize(_size);
}

void Tile::update(float _dt, const View& _view) {

    // Apply tile-view translation to the model matrix
    const auto& viewOrigin = _view.getPosition();
    m_modelMatrix[3][0] = m_tileOrigin.x - viewOrigin.x;
    m_modelMatrix[3][1] = m_tileOrigin.y - viewOrigin.y;

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

std::shared_ptr<Properties> Tile::getSelectionFeature(uint32_t _id) {
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
    }

    return m_memoryUsage;
}

}
