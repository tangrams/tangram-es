#include "tile.h"

#include "data/dataSource.h"
#include "scene/scene.h"
#include "scene/styleContext.h"
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
    // negative y coordinate: to change from y down to y up
    // (tile system has y down and gl context we use has y up).
    m_tileOrigin.y *= -1.0;

    auto mapBound = m_projection->MapBounds();
    auto mapSpan = mapBound.max.x - mapBound.min.x;

    m_tileOrigin.x += (mapSpan * _wrap);
}

void Tile::initGeometry(uint32_t _size) {
    //m_geometry.resize(_size);
}

void Tile::update(float _dt, const View& _view) {

    // Apply tile-view translation to the model matrix
    const auto& viewOrigin = _view.getPosition();
    m_modelMatrix[3][0] = m_tileOrigin.x - viewOrigin.x;
    m_modelMatrix[3][1] = m_tileOrigin.y - viewOrigin.y;

}

void Tile::resetState() {
    for (auto& entry : m_geometry) {
        if (!entry.second) { continue; }
        auto labelMesh = dynamic_cast<LabelMesh*>(entry.second.get());
        if (!labelMesh) { continue; }
        labelMesh->reset();
    }
}

void Tile::draw(const Style& _style, const View& _view) {

    auto& styleMesh = getMesh(_style);

    if (styleMesh) {
        auto& shader = _style.getShaderProgram();

        float zoomAndProxy = isProxy() ? -m_id.z : m_id.z;

        shader->setUniformMatrix4f("u_model", m_modelMatrix);
        shader->setUniformf("u_tile_origin", m_tileOrigin.x, m_tileOrigin.y, zoomAndProxy);

        styleMesh->draw(*shader);
    }
}

std::unique_ptr<VboMesh>& Tile::getMesh(const Style& _style) {
    // static std::unique_ptr<VboMesh> NONE = nullptr;
    // if (_style.getID() >= m_geometry.size()) { return NONE; }
    // return m_geometry[_style.getID()];
    return m_geometry[_style.getName()];
}

size_t Tile::getMemoryUsage() const {
    if (m_memoryUsage == 0) {
        for (auto& entry : m_geometry) {
            if (entry.second) {
                m_memoryUsage += entry.second->bufferSize();
            }
        }
    }

    return m_memoryUsage;
}

}
