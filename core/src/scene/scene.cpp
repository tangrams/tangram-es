#include "scene.h"

#include "util/stringsOp.h"

void Scene::addStyle(std::unique_ptr<Style> _style) {
    m_styles.push_back(std::move(_style));
}

void Scene::addDirectionalLight(std::unique_ptr<DirectionalLight> _dLight){
    
    if(_dLight->m_name == "abstractLight"){
        //  If the name is not declare add it to the array
        //
        
        _dLight->m_name = "directionalLights["+getString(m_directionalLights.size())+"]";
        
    } else {
        //  TODO:
        //        - This is for custom lights I guess
    }
    
    m_directionalLights.push_back(std::move(_dLight));
}
void Scene::addPointLight(std::unique_ptr<PointLight> _pLight){
    
    if(_pLight->m_name == "abstractLight"){
        //  If the name is not declare add it to the array
        //
        
        _pLight->m_name = "pointLights["+getString(m_pointLights.size())+"]";
        
    } else {
        //  TODO:
        //        - This is for custom lights I guess
    }
    
    m_pointLights.push_back(std::move(_pLight));
}

void Scene::addSpotLight(std::unique_ptr<SpotLight> _sLight){
    
    if(_sLight->m_name == "abstractLight"){
        //  If the name is not declare add it to the array
        //
        
        _sLight->m_name = "spotLights["+getString(m_spotLights.size())+"]";
        
    } else {
        //  TODO:
        //        - This is for custom lights I guess
    }
    
    m_spotLights.push_back(std::move(_sLight));
}
