#include "scene.h"

#include "gl/shaderProgram.h"
#include "platform.h"
#include "style/material.h"
#include "style/style.h"
#include "scene/dataLayer.h"
#include "scene/light.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "text/fontContext.h"
#include "util/mapProjection.h"
#include "view/view.h"

#include <atomic>
#include <algorithm>

namespace Tangram {

static std::atomic<int32_t> s_serial;

Scene::Scene() : id(s_serial++) {
    m_fontContext = std::make_shared<FontContext>();
    m_view = std::make_shared<View>();
    // For now we only have one projection..
    // TODO how to share projection with view?
    m_mapProjection.reset(new MercatorProjection());
}

Scene::~Scene() {}

const Style* Scene::findStyle(int _name) const {
    return  m_styles[_name].get();
}

int Scene::getStyleId(const std::string& _name) const {
    for (auto& style : m_styles) {
         if (style->getName() == _name) { return style->getID(); }
    }
    return -1;
}

int Scene::addStyleNameId(const std::string& _name) {
    int id = getStyleNameId(_name);

    if (id < 0) {
        m_styleNames.push_back(_name);
        return m_styleNames.size()-1;
    }
    return id;
}

int Scene::getStyleNameId(const std::string& _name) const {
    auto it = std::find(m_styleNames.begin(), m_styleNames.end(), _name);
    if (it == m_styleNames.end()) {
        return -1;
    }
    return it - m_styleNames.begin();
}

const Light* Scene::findLight(const std::string &_name) const {
    for (auto& light : m_lights) {
        if (light->getInstanceName() == _name) { return light.get(); }
    }
    return nullptr;
}

}
