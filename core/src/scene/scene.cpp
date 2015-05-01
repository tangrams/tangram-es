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

void Scene::addLight(std::unique_ptr<Light> _light) {

    // Avoid duplications
    if (m_lights.find(_light->getInstanceName()) != m_lights.end()) {
        logMsg("ERROR: Can't add the same light twice. Try using another the name instead.\n");
        return;
    }
    
    // Add light to shader programs for all styles
    for (auto& style : m_styles) {
        _light->injectOnProgram(style->getShaderProgram());
    }
    
    m_lights[_light->getInstanceName()] = std::move(_light);
}
