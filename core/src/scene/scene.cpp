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

void Scene::addLight(std::unique_ptr<Light> _light){
    
    if( _light->getType() == LIGHT_DIRECTIONAL &&   //  If is DIRECTIONAL and
        _light->getName() == "directionaLight"){    //  have the default name, can be set to be add to the array.
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

void Scene::injectLightning(){
    std::string lightsBlock = "";

    bool isLights = false;

    // TODO:    
    //          - What happen if the light is not in the array? the block need to be add with 
    //          - Add #ifndef to the blocks in order to avoid collisions between in/out array lights
    if(m_directionaLightCounter > 0){
        lightsBlock += DirectionalLight::getClassBlock()+"\n";
        lightsBlock += "#define NUM_DIRECTIONAL_LIGHTS " + getString(m_directionaLightCounter) + "\n";
        lightsBlock += "uniform DirectionalLight u_directionalLights[NUM_DIRECTIONAL_LIGHTS];\n\n";
    }

    if(m_pointLightCounter > 0){
        lightsBlock += PointLight::getClassBlock()+"\n";
        lightsBlock += "#define NUM_POINT_LIGHTS " + getString(m_pointLightCounter) + "\n";
        lightsBlock += "uniform PointLight u_pointLights[NUM_POINT_LIGHTS];\n\n";
    }

    if(m_spotLightCounter > 0){
        lightsBlock += SpotLight::getClassBlock()+"\n";
        lightsBlock += "#define NUM_SPOT_LIGHTS " + getString(m_spotLightCounter) + "\n";
        lightsBlock += "uniform SpotLight u_spotLights[NUM_SPOT_LIGHTS];\n\n";
    }

    //  After the headers are injected the calulatelitening function.
    lightsBlock += stringFromResource("lights.glsl");

    //  Inject the light block
    for(int i = 0; i < m_styles.size(); i++){
        m_styles[i]->getShaderProgram()->replaceAndRebuild("lighting",lightsBlock);
    }

    // TODO:    
    //          - improve injection system for not recompiling and replacing

    //  UNROLLED LOOP
    //
    //  This could be resolver more elegantly with for loops and ifdef inside the glsl code
    //  BUT we prove that for loops (even of arrays of one) are extremely slow on the iOS simulator
    //  BIG MISTERY
    //
    if(m_lights.size() > 0){
        std::string ligthsListBlock = "";

        for(int i = 0; i < m_lights.size(); i++){
            ligthsListBlock += "calculateLight("+m_lights[i]->getUniformName()+", eye, _ecPosition, _normal, amb, diff, spec);\n";
        }

        for(int i = 0; i < m_styles.size(); i++){
            m_styles[i]->getShaderProgram()->replaceAndRebuild("lights_calcualate_list",ligthsListBlock);
        }
    }
}
