#include "scene.h"

#include "platform.h"
#include "util/stringsOp.h"

#include "directionalLight.h"
#include "pointLight.h"
#include "spotLight.h"

Scene::Scene() {

}

void Scene::addStyle(std::unique_ptr<Style> _style) {
    m_styles.push_back(std::move(_style));
}

void Scene::addLight(std::shared_ptr<Light> _light, InjectionType _type) {

    //  Avoid duplications
    if (m_lights.find(_light->getName()) != m_lights.end()) {
        logMsg("ERROR: Can't add the same light twice. Try using another the name instead.\n");
        return;
    }
    
    for (auto& style : m_styles) {
        _light->injectOnProgram(style->getShaderProgram(), _type);
        
        if( _light->getInjectionType() == FRAGMENT){
            style->getShaderProgram()->addSourceBlock("defines", "#define TANGRAM_FRAGMENT_LIGHTS\n", false);
        }
        

        if( _light->getInjectionType() == VERTEX){
            style->getShaderProgram()->addSourceBlock("defines", "#define TANGRAM_VERTEX_LIGHTS\n", false);
        }
        
        
    }
    
    m_lights[_light->getName()] = _light;
}
