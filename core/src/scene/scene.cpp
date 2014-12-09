#include "scene.h"

#include "platform.h"
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
    std::string lights = "";

    bool isLights = false;

    if(m_directionalLights.size() > 0){
        lights += m_directionalLights[0]->getTransform()+"\n";
        lights += "#define NUM_DIRECTIONAL_LIGHTS " + getString(m_directionalLights.size()) + "\n";
        lights += "uniform DirectionalLight u_directionalLights[NUM_DIRECTIONAL_LIGHTS];\n\n";
        isLights = true;
    }

    if(m_pointLights.size() > 0){
        lights += m_pointLights[0]->getTransform()+"\n";
        lights += "#define NUM_POINT_LIGHTS " + getString(m_pointLights.size()) + "\n";
        lights += "uniform PointLight u_pointLights[NUM_POINT_LIGHTS];\n\n";
        isLights = true;
    }

    if(m_spotLights.size() > 0){
        lights += m_spotLights[0]->getTransform();
        lights += "\n#define NUM_SPOT_LIGHTS " + getString(m_spotLights.size()) + "\n";
        lights += "uniform SpotLight u_spotLights[NUM_SPOT_LIGHTS];\n\n";
        isLights = true;
    }

    if(isLights){
        // lights += stringFromResource("modules/ligths.glsl");
        lights += stringFromResource("lights.glsl");

        for(int i = 0; i < m_styles.size(); i++){
            m_styles[i]->getShaderProgram()->replaceAndRebuild("lighting",lights);
        }

        //  This could be resolver more elegantly with for loops and ifdef inside the glsl code
        //  BUT we prove that for loops (even of arrays of one) are extremely slow on the iOS simulator
        //  BIG MISTERY
        //
        if(m_directionalLights.size() > 0){
            std::string dirLigths = "";

            for(int i = 0; i < m_directionalLights.size(); i++){
                dirLigths += "calculateLight("+m_directionalLights[i]->getUniformName()+", eye, _ecPosition, _normal, amb, diff, spec);\n";
            }

            for(int i = 0; i < m_styles.size(); i++){
                m_styles[i]->getShaderProgram()->replaceAndRebuild("directional_lights",dirLigths);
            }
        } 

        if(m_pointLights.size() > 0){
            std::string pntLigths = "";
            
            for(int i = 0; i < m_pointLights.size(); i++){
                pntLigths += "calculateLight("+m_pointLights[i]->getUniformName()+", eye, _ecPosition, _normal, amb, diff, spec);\n";
            }

            for(int i = 0; i < m_styles.size(); i++){
                m_styles[i]->getShaderProgram()->replaceAndRebuild("point_lights",pntLigths);
            }
        }

        if(m_spotLights.size() > 0){
            std::string sptLigths = "";
            
            for(int i = 0; i < m_spotLights.size(); i++){
                sptLigths += "calculateLight("+m_spotLights[i]->getUniformName()+", eye, _ecPosition, _normal, amb, diff, spec);\n";
            }

            for(int i = 0; i < m_styles.size(); i++){
                m_styles[i]->getShaderProgram()->replaceAndRebuild("spot_ligths",sptLigths);
            }
        }
    }
}
