#include "mapTile.h"

MapTile::MapTile(TileID _id, const Projection& _projection) : m_id(_id) {

    glm::dvec4 tileBounds = _projection.tileBounds(_address);
    m_tileOrigin = glm::dvec2(tileBounds.x, tileBounds.y);
    m_modelMatrix = glm::translate(glm::dmat4(1.0), tileBounds.x, tileBounds.y, 0.0); //Use the lower-left corner for the 'model position'

}

MapTile::~MapTile() {

    m_geometry.clear();

}

void MapTile::addGeometry(const Style& _style, std::unique_ptr<VboMesh>&& _mesh) {

    m_geometry[_style.getStyleName()] = std::unique_ptr<VboMesh>(_mesh); // Move-construct a unique_ptr at the value associated with the given style

}

void MapTile::draw(const Style& _style, const glm::dmat4& _viewProjMatrix) {

    std::unique_ptr<VboMesh> styleMesh = m_geometry[_style.getStyleName()];

    if (styleMesh) {

        ShaderProgram& shader = _style.getShaderProgram();

        glm::mat4 modelViewProjMatrix = m_modelMatrix * _viewProjMatrix;
        shader.setUniformMatrix4f("u_modelViewProj", glm::value_ptr(modelViewProjMatrix));

        styleMesh->draw(shader);

    }

}
