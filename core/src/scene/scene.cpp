#include "scene.h"

#include "platform.h"
#include "util/stringsOp.h"

#include "directionalLight.h"
#include "pointLight.h"
#include "spotLight.h"

Scene::Scene(){

}

void Scene::addStyle(std::unique_ptr<Style> _style) {
    m_styles.push_back(std::move(_style));
}

void Scene::addLight(std::shared_ptr<Light> _light){
    
    //  Add (inject) need code blocks (defines, structs, functions and instances) 
    //  to compute this light.
    //  
    //  NOTE:   still the MAIN "calculateLighting" function (that computes all the lights) 
    //          HAVE TO be add at the very end
    //
    for(int i = 0; i < m_styles.size(); i++){
        _light->injectOnProgram( m_styles[i]->getShaderProgram() );
    }

    m_lights.push_back(_light);
}

void Scene::buildShaders(){
    
    //  INJECT the MAIN "calculateLighting" function (that computes all the lights)
    //
    std::string calculateLightBlock = stringFromResource("lights.glsl"); // Main "calculateLighting()" function
    if (m_lights.size() > 0){
        std::string ligthsListBlock = "";
        for(int i = 0; i < m_lights.size(); i++){
            ligthsListBlock += m_lights[i]->getInstanceComputeBlock();
        }
        replaceString(calculateLightBlock,"#pragma tangram: lights_to_compute",ligthsListBlock); 
    }

    for(int i = 0; i < m_styles.size(); i++){
        m_styles[i]->getShaderProgram()->addBlock("lighting", calculateLightBlock+"\n");
    }


    //  COMPILE ALL SHADERS
    //
    for(int i = 0; i < m_styles.size(); i++){
        m_styles[i]->getShaderProgram()->build();
    }
}
