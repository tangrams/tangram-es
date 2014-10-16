#include "mapTile.h"
#include "style/style.h"
#include "util/tileID.h"

#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"

MapTile::MapTile(TileID _id, const MapProjection& _projection) : m_id(_id) {

    glm::dvec4 tileBounds = _projection.TileBounds(_id);
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

    //logMsg("Drawing mapTile %d/%d/%d\n", m_id.z, m_id.x, m_id.y);

    if (styleMesh) {

        //logMsg("    Drawing style %s\n", _style.getStyleName().c_str());

        std::shared_ptr<ShaderProgram> shader = _style.getShaderProgram();

        glm::dmat4 modelViewProjMatrix = _viewProjMatrix * m_modelMatrix;

        //TODO: better cast to float
        glm::mat4 fmvp;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                fmvp[i][j] = (float)modelViewProjMatrix[i][j];
            }
        }

        //logMsg("    MVP matrix: %s\n", glm::to_string(fmvp).c_str());

        shader->setUniformMatrix4f("u_modelViewProj", glm::value_ptr(fmvp));

        styleMesh->draw(shader);

    }

}
