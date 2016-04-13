#include "scene.h"

#include "gl/shaderProgram.h"
#include "platform.h"
#include "style/material.h"
#include "style/style.h"
#include "scene/dataLayer.h"
#include "scene/light.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "util/mapProjection.h"
#include "view/view.h"
#include "util/util.h"

#include <atomic>
#include <algorithm>

#define COMPONENT_PATH_DELIMITER '.'

namespace Tangram {

static std::atomic<int32_t> s_serial;

Scene::Scene(std::string scene) : id(s_serial++), m_scene(scene) {
    m_view = std::make_shared<View>();
    // For now we only have one projection..
    // TODO how to share projection with view?
    m_mapProjection.reset(new MercatorProjection());
}

Scene::Scene(std::vector<UpdateValue> updates, std::string scene) :
    Scene(scene)
{
    m_updates = updates;
}

Scene::~Scene() {}

const Style* Scene::findStyle(const std::string& _name) const {

    for (auto& style : m_styles) {
        if (style->getName() == _name) { return style.get(); }
    }
    return nullptr;

}

int Scene::addIdForName(const std::string& _name) {
    int id = getIdForName(_name);

    if (id < 0) {
        m_names.push_back(_name);
        return m_names.size() - 1;
    }
    return id;
}

int Scene::getIdForName(const std::string& _name) const {
    auto it = std::find(m_names.begin(), m_names.end(), _name);
    if (it == m_names.end()) {
        return -1;
    }
    return it - m_names.begin();
}

const Light* Scene::findLight(const std::string &_name) const {
    for (auto& light : m_lights) {
        if (light->getInstanceName() == _name) { return light.get(); }
    }
    return nullptr;
}

bool Scene::texture(const std::string& textureName, std::shared_ptr<Texture>& texture) const {
    auto texIt = m_textures.find(textureName);

    if (texIt == m_textures.end()) {
        return false;
    }

    texture = texIt->second;

    return true;
}

void Scene::queueComponentUpdate(std::string componentPath, std::string value) {
    std::vector<std::string> splitPath = splitString(componentPath, COMPONENT_PATH_DELIMITER);
    m_updates.push_back({ splitPath, value });
}

}
