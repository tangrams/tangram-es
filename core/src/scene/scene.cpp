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
    
    // For the first light added, add the main lighting function
    if (m_lights.size() == 0) {
        std::string vertexLightBlock = stringFromResource("lights_vert.glsl");
        std::string fragmentLightBlock = stringFromResource("lights_frag.glsl");
        
        for (auto& style : m_styles) {
            style->getShaderProgram()->addSourceBlock("_vertex_lighting", vertexLightBlock+"\n");
            style->getShaderProgram()->addSourceBlock("_fragment_lighting", fragmentLightBlock+"\n");
        }
    }


    //  Avoid duplications
    if (m_lights.find(_light->getName()) == m_lights.end()) {
        for (auto& style : m_styles) {
            _light->injectOnProgram(style->getShaderProgram(), _type);
        }
        m_lights[_light->getName()] = _light;
    }
}
