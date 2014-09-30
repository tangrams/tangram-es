#include "mapTile.h"

MapTile::MapTile(glm::ivec3 _address, const Projection& _projection) : m_address(_address) {

    glm::vec4 tileBounds = _projection.tileBounds(_address);
    m_tileOrigin = glm::vec3(tileBounds.x, tileBounds.y);
    m_modelMatrix = glm::translate(glm::mat4(1.0), tileBounds.x, tileBounds.y, 0); //Use the lower-left corner for the 'model position'

}

MapTile::~MapTile() {

    m_geometry.clear();

}

void MapTile::addGeometry(const Style& _style, unique_ptr<VboMesh>&& _mesh) {

    m_geometry[_style] = unique_ptr<VboMesh>(_mesh); // Move-construct a unique_ptr at the value associated with the given style

}

void MapTile::draw(const Style& _style, const glm::mat4& _viewProjMatrix) {

    std::unique_ptr<VboMesh> styleMesh = m_geometry[_style];

    if (styleMesh) {

        ShaderProgram& shader = _style.getShaderProgram();

        glm::mat4 modelViewProjMatrix = m_modelMatrix * _viewProjMatrix;
        shader.setUniform("u_modelViewProj", modelViewProjMatrix);

        styleMesh->draw(shader);

    }

}
