#include "scene.h"

#include "platform.h"
#include "util/stringsOp.h"

#include "directionalLight.h"
#include "pointLight.h"
#include "spotLight.h"

Scene::Scene():m_isDirectionalLights(false),m_isPointLights(false),m_isSpotLights(false){

}

void Scene::addStyle(std::unique_ptr<Style> _style) {
    m_styles.push_back(std::move(_style));
}

void Scene::addLight(std::shared_ptr<Light> _light){
    
    if( _light->getType() == LIGHT_DIRECTIONAL){
        m_isDirectionalLights = true;
    } else if( _light->getType() == LIGHT_POINT){
        m_isPointLights = true;
    } else if( _light->getType() == LIGHT_SPOT ){
        m_isSpotLights = true;
    }

    //  TODO: 
    //          - check for name and #defines collitions
    
    m_lights.push_back(_light);
}

void Scene::buildShaders(){
    
    //  LIGHTENING INJECTION
    //
    std::string lightsDefines = "";     //  special "#DEFINE LIGHT_..." flags 
    std::string lightsClassBlock = "";  //  Needed structs and "calculateLight()"" functions for that struct
    std::string lightsInstance = "";    //  Uniform / Global declaration of each instance
    std::string lightsAssing = "";      //  Assign values to non-dynamic lights
    std::string calculateLightBlock = stringFromResource("lights.glsl"); // Main "calculateLighting()" function

    if(m_isDirectionalLights){
        lightsClassBlock += DirectionalLight::getClassBlock();
    }

    if(m_isPointLights){
        lightsClassBlock += PointLight::getClassBlock();
    }

    if(m_isSpotLights){
        lightsClassBlock += SpotLight::getClassBlock();
    }
    
    if (m_lights.size() > 0){
        std::string ligthsListBlock = "";
        for(int i = 0; i < m_lights.size(); i++){
            lightsDefines += m_lights[i]->getInstanceDefinesBlock();
            lightsInstance += m_lights[i]->getInstanceBlock();
            lightsAssing += m_lights[i]->getInstanceAssignBlock();
            ligthsListBlock += m_lights[i]->getInstanceComputeBlock();
        }
        replaceString(calculateLightBlock,"#pragma tangram: lights_to_compute",ligthsListBlock); 
    }

    //  Inject the light block
    for(int i = 0; i < m_styles.size(); i++){
        m_styles[i]->getShaderProgram()->addBlock("lighting",   lightsDefines+"\n"+
                                                                lightsClassBlock+"\n"+
                                                                lightsInstance+"\n"+
                                                                calculateLightBlock+"\n");

        m_styles[i]->getShaderProgram()->addBlock("lighting_non_dynamic_assing", lightsAssing);
    }

    //  COMPILE ALL SHADERS
    //
    for(int i = 0; i < m_styles.size(); i++){
        m_styles[i]->getShaderProgram()->build();
    }
}
