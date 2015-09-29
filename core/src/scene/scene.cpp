#include "scene.h"

#include "platform.h"
#include "style/style.h"
#include "text/fontContext.h"

#include <atomic>

namespace Tangram {

static std::atomic<int32_t> s_serial;

Scene::Scene() : id(s_serial++) {
    m_fontContext = std::make_shared<FontContext>();
}
Scene::~Scene() {}

const Style* Scene::findStyle(const std::string &_name) const {
    for (auto& style : m_styles) {
        if (style->getName() == _name) { return style.get(); }
    }
    return nullptr;
}

const Light* Scene::findLight(const std::string &_name) const {
    for (auto& light : m_lights) {
        if (light->getInstanceName() == _name) { return light.get(); }
    }
    return nullptr;
}

}
