#include "scene.h"

void Scene::addStyle(std::unique_ptr<Style> _style) {

    m_styles.push_back(std::move(_style));

}

