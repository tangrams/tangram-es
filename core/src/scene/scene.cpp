#include "scene.h"

#include "platform.h"
#include "directionalLight.h"
#include "pointLight.h"
#include "spotLight.h"

Scene::Scene() {

}

void Scene::addStyle(std::unique_ptr<Style> _style) {
    m_styles.push_back(std::move(_style));
}

void Scene::removeStyle(const std::string& _name) {
    
    for (auto iter = m_styles.begin(); iter != m_styles.end(); ++iter) {
        if ((*iter)->getName() == _name) {
            m_styles.erase(iter);
            break;
        }
    }
}

void Scene::addLight(std::shared_ptr<Light> _light, InjectionType _type) {

    // Avoid duplications
    if (m_lights.find(_light->getName()) != m_lights.end()) {
        logMsg("ERROR: Can't add the same light twice. Try using another the name instead.\n");
        return;
    }
    
    // Add light to shader programs for all styles
    for (auto& style : m_styles) {
        _light->injectOnProgram(style->getShaderProgram(), _type);
    }
    
    m_lights[_light->getName()] = _light;
}
