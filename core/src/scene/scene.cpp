#include "scene.h"

#include "data/dataSource.h"
#include "gl/shaderProgram.h"
#include "platform.h"
#include "scene/dataLayer.h"
#include "scene/light.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "style/material.h"
#include "style/style.h"
#include "text/fontContext.h"
#include "util/mapProjection.h"
#include "util/util.h"
#include "view/view.h"

#include <atomic>
#include <algorithm>

#define COMPONENT_PATH_DELIMITER '.'

namespace Tangram {

static std::atomic<int32_t> s_serial;

Scene::Scene(const std::string& _path)
    : id(s_serial++),
      m_path(_path),
      m_fontContext(std::make_shared<FontContext>()) {

    m_view = std::make_shared<View>();
    // For now we only have one projection..
    // TODO how to share projection with view?
    m_mapProjection.reset(new MercatorProjection());
}

Scene::Scene(const Scene& _other)
    : Scene(_other.path()) {

    m_config = _other.m_config;
    m_updates = _other.m_updates;
    m_view = _other.m_view;
    m_fontContext = _other.m_fontContext;
}

Scene::~Scene() {}

const Style* Scene::findStyle(const std::string& _name) const {

    for (auto& style : m_styles) {
        if (style->getName() == _name) { return style.get(); }
    }
    return nullptr;

}

Style* Scene::findStyle(const std::string& _name) {

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

std::shared_ptr<Texture> Scene::getTexture(const std::string& textureName) const {
    auto texIt = m_textures.find(textureName);
    if (texIt == m_textures.end()) {
        return nullptr;
    }
    return texIt->second;
}

void Scene::queueUpdate(std::string path, std::string value) {
    auto keys = splitString(path, COMPONENT_PATH_DELIMITER);
    std::lock_guard<std::mutex> lock(m_updatesMutex);
    m_updates.push_back({ keys, value });
}

std::shared_ptr<DataSource> Scene::getDataSource(const std::string& name) {
    auto it = std::find_if(m_dataSources.begin(), m_dataSources.end(),
                           [&](auto& s){ return s->name() == name; });
    if (it != m_dataSources.end()) {
        return *it;
    }
    return nullptr;
}

}
