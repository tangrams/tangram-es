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
    
    m_modelMatrix = glm::dmat4(1.0);
    
    // Translate model matrix to origin of tile
    m_modelMatrix = glm::translate(m_modelMatrix, glm::dvec3(m_tileOrigin.x, m_tileOrigin.y, 0.0));
    
    // Scale model matrix to size of tile
    m_modelMatrix = glm::scale(m_modelMatrix, glm::dvec3(m_scale));

}

MapTile::~MapTile() {

    m_geometry.clear();

}

void MapTile::addGeometry(const Style& _style, std::unique_ptr<VboMesh> _mesh) {

    m_geometry[_style.getName()] = std::move(_mesh); // Move-construct a unique_ptr at the value associated with the given style

}

void MapTile::draw(const Style& _style, const View& _view) {

    const std::unique_ptr<VboMesh>& styleMesh = m_geometry[_style.getName()];

    std::shared_ptr<ShaderProgram> shader = _style.getShaderProgram();

    glm::dmat4 modelViewProjMatrix = _view.getViewProjectionMatrix() * m_modelMatrix;

    // NOTE : casting to float, but loop over the matrix values
    double* first = &modelViewProjMatrix[0][0];
    std::vector<float> fmvp(first, first + 16);

    shader->setUniformMatrix4f("u_modelViewProj", &fmvp[0]);

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

bool MapTile::hasGeometry() {
    return (m_geometry.size() != 0);
}

bool MapTile::hasGeometry(std::string _styleName) {
    return (m_geometry.find(_styleName) != m_geometry.end()) ? true : false;
}

