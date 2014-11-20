#include "mapTile.h"
#include "style/style.h"
#include "util/tileID.h"

#include "scene/scene.h"
#include "util/stringsOp.h"

#define GLM_FORCE_RADIANS
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

void MapTile::draw( Scene& _scene, const Style& _style, const glm::dmat4& _viewProjMatrix){
    const std::unique_ptr<VboMesh>& styleMesh = m_geometry[_style.getName()];
    
    if (styleMesh) {
        
        std::shared_ptr<ShaderProgram> shader = _style.getShaderProgram();
        
        glm::dmat4 modelViewProjMatrix = _viewProjMatrix * m_modelMatrix;
        
        // NOTE : casting to float, but loop over the matrix values
        double* first = &modelViewProjMatrix[0][0];
        std::vector<float> fmvp(first, first + 16);
        
        shader->setUniformMatrix4f("u_modelViewProj", &fmvp[0]);
        shader->setUniformf("u_time", ((float)clock())/CLOCKS_PER_SEC);
        
        for (int i = 0; i < _scene.getLights().size(); i++){
            
            //  Strip this in orhder to change the position of light for the relative position of it to this specific tile
            //
//            shader->setLightUniform(*_scene.getLights()[i],i);
            //
            //  or
            //
            
            glm::vec3 relativeLight = _scene.getLights()[i]->m_position;
            
            shader->setLightUniform("u_lights[" + getString(i) + "].ambient", i, _scene.getLights()[i]->m_ambient);
            shader->setLightUniform("u_lights[" + getString(i) + "].diffuse", i, _scene.getLights()[i]->m_diffuse);
            shader->setLightUniform("u_lights[" + getString(i) + "].specular", i, _scene.getLights()[i]->m_specular);
            shader->setLightUniform("u_lights[" + getString(i) + "].position", i, smPos);
            shader->setLightUniform("u_lights[" + getString(i) + "].halfVector", i, _scene.getLights()[i]->m_halfVector);
            shader->setLightUniform("u_lights[" + getString(i) + "].direction", i, _scene.getLights()[i]->m_direction);
            shader->setLightUniform("u_lights[" + getString(i) + "].spotExponent", i, _scene.getLights()[i]->m_spotExponent);
            shader->setLightUniform("u_lights[" + getString(i) + "].spotCutoff", i, _scene.getLights()[i]->m_spotCutoff);
            shader->setLightUniform("u_lights[" + getString(i) + "].spotCosCutoff", i, _scene.getLights()[i]->m_spotCosCutoff);
            shader->setLightUniform("u_lights[" + getString(i) + "].constantAttenuation", i, _scene.getLights()[i]->m_constantAttenuation);
            shader->setLightUniform("u_lights[" + getString(i) + "].linearAttenuation", i, _scene.getLights()[i]->m_linearAttenuation);
            shader->setLightUniform("u_lights[" + getString(i) + "].quadraticAttenuation", i, _scene.getLights()[i]->m_quadraticAttenuation);

            
        }
        
        styleMesh->draw(shader);
    }
}

