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

namespace Tangram {

static std::atomic<int32_t> s_serial;

Scene::Scene(std::string path) : id(s_serial++), m_path(path) {
    m_view = std::make_shared<View>();
    // For now we only have one projection..
    // TODO how to share projection with view?
    m_mapProjection.reset(new MercatorProjection());
}

Scene::Scene(std::string path, std::unordered_map<StyleComponent, StyleComponents> userDefined) :
    Scene(path)
{
    m_userDefinedValues = userDefined;
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

bool Scene::getComponentValue(StyleComponent component, const std::string& componentPath, std::string& value) {
    return tryFind(m_userDefinedValues[component], componentPath, value);
}

void Scene::setComponent(std::string componentPath, std::string value) {
    static const std::map<std::string, StyleComponent> components = {
        {"scene", StyleComponent::scene},
        {"global", StyleComponent::global},
        {"cameras", StyleComponent::cameras},
        {"lights", StyleComponent::lights},
        {"textures", StyleComponent::textures},
        {"styles", StyleComponent::styles},
        {"sources", StyleComponent::sources},
        {"layers", StyleComponent::layers}
    };

    std::vector<std::string> splitPath = splitString(componentPath, COMPONENT_PATH_DELIMITER);

    if (splitPath.size() < 2) {
        return;
    }

    StyleComponent component;
    if (!tryFind(components, splitPath[0], component)) {
        return;
    }

    m_userDefinedValues[component][componentPath] = value;

#if 0
    for (auto styleComponent : m_userDefinedValues) {
        for (auto components : styleComponent.second) {
            std::string path;
            for (auto p : components.second.path) {
                path += "/" + p;
            }
            LOG("[%d] - %s @ %s", styleComponent.first,
                components.second.value.c_str(), path.c_str());
        }
    }
#endif
}

}
