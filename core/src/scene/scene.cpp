#include "scene.h"

#include "util/stringsOp.h"

void Scene::addStyle(std::unique_ptr<Style> _style) {
    m_styles.push_back(std::move(_style));
}

void Scene::addLight(std::unique_ptr<DirectionalLight> _dLight){
    
    if(_dLight->getName() == "abstractLight"){

        //  If the name is not declare add it to the array
        _dLight->setName("directionalLights",m_directionalLights.size());
        m_directionalLights.push_back(std::move(_dLight));
        
    } else {
        //  TODO:
        //        - Custom lights
    }
}

void Scene::addLight(std::unique_ptr<PointLight> _pLight){
    
    if(_pLight->getName() == "abstractLight"){

        //  If the name is not declare add it to the array
        _pLight->setName("pointLights",m_pointLights.size());
        m_pointLights.push_back(std::move(_pLight));

    } else {
        //  TODO:
        //        - Custom lights
    }
}

void Scene::addLight(std::unique_ptr<SpotLight> _sLight){
    
    if(_sLight->getName() == "abstractLight"){

        //  If the name is not declare add it to the array
        _sLight->setName("spotLights",m_spotLights.size());
        m_spotLights.push_back(std::move(_sLight));

    } else {
        //  TODO:
        //        - Custom lights
    }
}

void Scene::injectLightning(){
    std::string glsl = "";

    if(m_directionalLights.size() > 0){
        glsl += "\n #define NUM_DIRECTIONAL_LIGHTS " + getString(m_directionalLights.size()) + "\n";
    }

    if(m_pointLights.size() > 0){
        glsl += "\n #define NUM_POINT_LIGHTS " + getString(m_pointLights.size()) + "\n";
    }

    if(m_spotLights.size() > 0){
        glsl += "\n #define NUM_SPOT_LIGHTS " + getString(m_spotLights.size()) + "\n";
    }

    
}
