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

const Style* Scene::findStyle(const std::string &_name) const {
    for (auto& style : m_styles) {
        if (style->getName() == _name) { return style.get(); }
    }
    for (auto& style : m_stencilStyles) {
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

bool Scene::containsStyleWithBlend(Blending _blend) const {
     for (auto& style : m_styles) {
        if (style->blendMode() == _blend) {
            return true;
        }
    }
    return false;
}

}
