#include "scene.h"

#include "platform.h"
#include "util/stringsOp.h"

#include "directionalLight.h"
#include "pointLight.h"
#include "spotLight.h"

Scene::Scene():m_directionaLightCounter(0),m_pointLightCounter(0),m_spotLightCounter(0){

}

void Scene::addStyle(std::unique_ptr<Style> _style) {
    m_styles.push_back(std::move(_style));
}

void Scene::addLight(std::shared_ptr<Light> _light){
    
    if( _light->getType() == LIGHT_DIRECTIONAL &&   //  If is DIRECTIONAL and
        _light->getName() == "directionalLight"){    //  have the default name, can be set to be add to the array.
        _light->setIndexPos(m_directionaLightCounter);
        m_directionaLightCounter++;
    } else if( _light->getType() == LIGHT_POINT &&  //  If is POINT and
        _light->getName() == "pointLight"){    //  have the default name, can be set to be add to the array.
        _light->setIndexPos(m_pointLightCounter);
        m_pointLightCounter++;
    } else if( _light->getType() == LIGHT_SPOT &&  //  If is POINT and
        _light->getName() == "spotLight"){    //  have the default name, can be set to be add to the array.
        _light->setIndexPos(m_spotLightCounter);
        m_spotLightCounter++;
    }

    //  Beside the nature of the ligth add it to the vector
    //  We pre supose that the light already know how to be injected as uniform (if is need)
    m_lights.push_back(std::move(_light));
}

void Scene::buildShaders(){
    
    // TODO:    
    //          - What happen if the light is not in the array? the block need to be add with 
    //          - Add #ifndef to the blocks in order to avoid collisions between in/out array lights
    
    //  LIGHTENING INJECTION
    //
    std::string lightsDefines = ""; //  "#DEFINE LIGHT_..."
    std::string lightsBlock = "";   //  Needed structs and "calculateLight()"" functions for that struct
    std::string lightsUniforms = "";//  Uniform declaration
    std::string calculateLightBlock = stringFromResource("lights.glsl"); // Main "calculateLighting()" function

    if(m_directionaLightCounter > 0){
        lightsDefines += "#define NUM_DIRECTIONAL_LIGHTS " + getString(m_directionaLightCounter) + "\n";
        lightsBlock += DirectionalLight::getClassBlock()+"\n";
        lightsUniforms += "uniform DirectionalLight u_directionalLights[NUM_DIRECTIONAL_LIGHTS];\n";
    }

    if(m_pointLightCounter > 0){
        lightsDefines += "#define NUM_POINT_LIGHTS " + getString(m_pointLightCounter) + "\n";
        lightsBlock += PointLight::getClassBlock()+"\n";
        lightsUniforms += "uniform PointLight u_pointLights[NUM_POINT_LIGHTS];\n";
    }

    if(m_spotLightCounter > 0){
        lightsDefines += "#define NUM_SPOT_LIGHTS " + getString(m_spotLightCounter) + "\n";
        lightsBlock += SpotLight::getClassBlock()+"\n";
        lightsUniforms += "uniform SpotLight u_spotLights[NUM_SPOT_LIGHTS];\n";
    }

    
    if (m_lights.size() > 0){
        std::string ligthsListBlock = "";
        for(int i = 0; i < m_lights.size(); i++){
            lightsDefines += m_lights[i]->getDefinesBlock();
            ligthsListBlock += "calculateLight("+m_lights[i]->getUniformName()+", eye, _ecPosition, _normal, amb, diff, spec);\n";
        }
        replaceString(calculateLightBlock,"#pragma tangram: lights_unrol_loop",ligthsListBlock); 
    }

    //  Inject the light block
    for(int i = 0; i < m_styles.size(); i++){
        m_styles[i]->getShaderProgram()->addBlock("lighting",   lightsDefines+"\n"+
                                                                lightsBlock+"\n"+
                                                                lightsUniforms+"\n"+
                                                                calculateLightBlock+"\n");
    }

    //  COMPILE ALL SHADERS
    //
    for(int i = 0; i < m_styles.size(); i++){
        m_styles[i]->getShaderProgram()->build();
    }
}
