#include "scene.h"

#include "platform.h"
#include "style/style.h"

Scene::Scene() {

}

void Scene::addStyle(std::unique_ptr<Style> _style) {
    m_styles.push_back(std::move(_style));
}

void Scene::addLight(std::unique_ptr<Light> _light) {

    // Avoid duplications
    const std::string& name = _light->getInstanceName();
    for (auto& light : m_lights) {
        if (light->getInstanceName() == name) {
            logMsg("WARNING: Found multiple lights with the name \"%s\"; all but one will be ignored.\n", name.c_str());
            return;
        }
    }

    m_lights.push_back(std::move(_light));

}
