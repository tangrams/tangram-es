#include "mapTile.h"
#include "style/style.h"
#include "view/view.h"
#include "util/tileID.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

MapTile::MapTile(TileID _id, const MapProjection& _projection) : m_id(_id),  m_projection(&_projection) {

    glm::dvec4 bounds = _projection.TileBounds(_id); // [x: xmin, y: ymin, z: xmax, w: ymax]
    
    m_scale = 0.5 * glm::abs(bounds.x - bounds.z);
    m_inverseScale = 1.0/m_scale;
    
    // negative y coordinate: to change from y down to y up (tile system has y down and gl context we use has y up).
    m_tileOrigin = glm::dvec2(0.5*(bounds.x + bounds.z), -0.5*(bounds.y + bounds.w));
    
    m_modelMatrix = glm::mat4(1.0);
    
    // Scale model matrix to size of tile
    m_modelMatrix = glm::scale(m_modelMatrix, glm::vec3(m_scale));

}

MapTile::~MapTile() {

    m_geometry.clear();

}

void MapTile::addGeometry(const Style& _style, std::unique_ptr<VboMesh> _mesh) {

    m_geometry[_style.getName()] = std::move(_mesh); // Move-construct a unique_ptr at the value associated with the given style

}

void MapTile::draw(const Style& _style, const View& _view) {
    

    const std::unique_ptr<VboMesh>& styleMesh = m_geometry[_style.getName()];
    
    if (styleMesh) {
        
        std::shared_ptr<ShaderProgram> shader = _style.getShaderProgram();

        // Apply tile-view translation to the model matrix
        const auto& viewOrigin = _view.getPosition();
        m_modelMatrix[3][0] = m_tileOrigin.x - viewOrigin.x;
        m_modelMatrix[3][1] = m_tileOrigin.y - viewOrigin.y;
        m_modelMatrix[3][2] = -viewOrigin.z;

        glm::mat4 modelViewMatrix = _view.getViewMatrix() * m_modelMatrix;
        glm::mat4 modelViewProjMatrix = _view.getViewProjectionMatrix() * m_modelMatrix;
        
        glm::mat3 normalMatrix = glm::mat3(modelViewMatrix); // Transforms surface normals into camera space
        normalMatrix = glm::transpose(glm::inverse(normalMatrix));
        
        shader->setUniformMatrix4f("u_modelView", glm::value_ptr(modelViewMatrix));
        shader->setUniformMatrix4f("u_modelViewProj", glm::value_ptr(modelViewProjMatrix));
        shader->setUniformMatrix3f("u_normalMatrix", glm::value_ptr(normalMatrix));

        // Set tile offset for proxy tiles
        float offset = 0;
        if (m_proxyCounter > 0) {
            offset = 1.0f + log((_view.s_maxZoom + 1) / (_view.s_maxZoom + 1 - m_id.z));
        } else {
            offset = 1.0f + log(_view.s_maxZoom + 2);
        }
        shader->setUniformf("u_tileDepthOffset", offset);

        styleMesh->draw(shader);
    }
}

bool MapTile::hasGeometry() {
    return (m_geometry.size() != 0);
}
