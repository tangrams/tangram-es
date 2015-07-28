#include "scene.h"

#include "platform.h"
#include "style/style.h"

namespace Tangram {

std::shared_ptr<Style> Scene::findStyle(const std::string &_name) {
    for (auto& style : m_styles) {
        if (style->getName() == _name) { return style; }
    }
    return nullptr;
}

std::shared_ptr<Light> Scene::findLight(const std::string &_name) {
    for (auto& light : m_lights) {
        if (light->getInstanceName() == _name) { return light; }
    }
    return nullptr;
}

}
