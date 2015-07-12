#include "mapTile.h"

#include "style/style.h"
#include "view/view.h"
#include "util/tileID.h"
#include "util/vboMesh.h"
#include "util/shaderProgram.h"
#include "text/fontContext.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"


MapTile::MapTile(TileID _id, const MapProjection& _projection)
    : m_id(_id),
      m_projection(&_projection) {

    BoundingBox bounds(_projection.TileBounds(_id));

    m_scale = 0.5 * bounds.width();
    m_inverseScale = 1.0/m_scale;
    
    m_tileOrigin = bounds.center();
    // negative y coordinate: to change from y down to y up (tile system has y down and gl context we use has y up).
    m_tileOrigin.y *= -1.0;

    // Init model matrix to size of tile
    m_modelMatrix = glm::scale(glm::mat4(1.0), glm::vec3(m_scale));
}

MapTile::~MapTile() {

}

void MapTile::addBatch(const Style& _style, std::unique_ptr<StyleBatch> _batch) {

    m_batches[_style.getName()] = std::move(_batch);

}

void MapTile::update(float _dt, const View& _view) {

    // Apply tile-view translation to the model matrix
    const auto& viewOrigin = _view.getPosition();
    m_modelMatrix[3][0] = m_tileOrigin.x - viewOrigin.x;
    m_modelMatrix[3][1] = m_tileOrigin.y - viewOrigin.y;
    m_modelMatrix[3][2] = -viewOrigin.z;

}

void MapTile::updateStyle(const Style& _style, float _dt, const View& _view) {
    glm::mat4 mvp = _view.getViewProjectionMatrix() * m_modelMatrix;
    
    auto it = m_batches.find(_style.getName());
    if (it != m_batches.end() && it->second) {
        it->second->update(mvp, _view, _dt);
    }
}

void MapTile::prepareStyle(const Style& _style) {
    auto it = m_batches.find(_style.getName());
    if (it != m_batches.end() && it->second) {
        it->second->prepare();
    }
}

void MapTile::draw(const Style& _style, const View& _view) {

    auto it = m_batches.find(_style.getName());
    if (it != m_batches.end() && it->second) {
        auto& batch = *it->second;
        
        std::shared_ptr<ShaderProgram> shader = _style.getShaderProgram();

        glm::mat4 modelViewMatrix = _view.getViewMatrix() * m_modelMatrix;
        glm::mat4 modelViewProjMatrix = _view.getViewProjectionMatrix() * m_modelMatrix;
        
        shader->setUniformMatrix4f("u_modelView", glm::value_ptr(modelViewMatrix));
        shader->setUniformMatrix4f("u_modelViewProj", glm::value_ptr(modelViewProjMatrix));
        shader->setUniformMatrix3f("u_normalMatrix", glm::value_ptr(_view.getNormalMatrix()));

        // Set the tile zoom level, using the sign to indicate whether the tile is a proxy
        shader->setUniformf("u_tile_zoom", m_proxyCounter > 0 ? -m_id.z : m_id.z);

        batch.draw(_view);
    }
}

