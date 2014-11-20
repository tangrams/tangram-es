#include "scene.h"

void Scene::addStyle(std::unique_ptr<Style> _style) {
    m_styles.push_back(std::move(_style));
}

void Scene::addLight(std::unique_ptr<Light> _light){
    m_lights.push_back(std::move(_light));
}