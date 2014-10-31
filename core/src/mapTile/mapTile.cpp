#include "mapTile.h"
#include "style/style.h"
#include "util/tileID.h"

#define GLM_FORCE_RADIANS
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

MapTile::MapTile(TileID _id, const MapProjection& _projection) : m_id(_id) {

    glm::dvec4 tileBounds = _projection.TileBounds(_id);
    // negative y coordinate: to change from y down to y up (tile system has y down and gl context we use has y up).
    m_tileOrigin = glm::dvec2(tileBounds.x, -tileBounds.y);
    m_modelMatrix = glm::translate(glm::dmat4(1.0), glm::dvec3(m_tileOrigin.x, m_tileOrigin.y, 0.0)); //Use the upper-left corner for the 'model position'

}

MapTile::~MapTile() {

    m_geometry.clear();

}

void MapTile::addGeometry(const Style& _style, std::unique_ptr<VboMesh> _mesh) {

    m_geometry[_style.getName()] = std::move(_mesh); // Move-construct a unique_ptr at the value associated with the given style

}

void MapTile::draw(const Style& _style, const glm::dmat4& _viewProjMatrix) {

    const std::unique_ptr<VboMesh>& styleMesh = m_geometry[_style.getName()];

    if (styleMesh) {

        std::shared_ptr<ShaderProgram> shader = _style.getShaderProgram();

        glm::dmat4 modelViewProjMatrix = _viewProjMatrix * m_modelMatrix;

        // NOTE : casting to float, but loop over the matrix values  
        double* first = &modelViewProjMatrix[0][0];
        std::vector<float> fmvp(first, first + 16);

        shader->setUniformMatrix4f("u_modelViewProj", &fmvp[0]);

        styleMesh->draw(shader);
    }
}

