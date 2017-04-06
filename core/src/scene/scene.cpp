#include "scene/scene.h"

#include "data/tileSource.h"
#include "gl/shaderProgram.h"
#include "platform.h"
#include "scene/dataLayer.h"
#include "scene/light.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "selection/featureSelection.h"
#include "style/material.h"
#include "style/style.h"
#include "text/fontContext.h"
#include "util/mapProjection.h"
#include "util/util.h"
#include "util/url.h"
#include "view/view.h"

#include <algorithm>

namespace Tangram {

static std::atomic<int32_t> s_serial;

Scene::Scene(std::shared_ptr<const Platform> _platform, const Url& _url)
    : id(s_serial++),
      m_url(_url),
      m_fontContext(std::make_shared<FontContext>(_platform)),
      m_featureSelection(std::make_unique<FeatureSelection>()) {

    // For now we only have one projection..
    // TODO how to share projection with view?
    m_mapProjection.reset(new MercatorProjection());
}

Scene::Scene(std::shared_ptr<const Platform> _platform, const std::string& _yaml, const Url& _url)
    : id(s_serial++),
      m_fontContext(std::make_shared<FontContext>(_platform)),
      m_featureSelection(std::make_unique<FeatureSelection>()) {

    m_url = _url;
    m_yaml = _yaml;

    m_mapProjection.reset(new MercatorProjection());
}

void Scene::copyConfig(const Scene& _other) {

    m_featureSelection.reset(new FeatureSelection());

    m_config = _other.m_config;
    m_fontContext = _other.m_fontContext;

    m_url = _other.m_url;
    m_yaml = _other.m_yaml;

    m_globalRefs = _other.m_globalRefs;

    m_mapProjection.reset(new MercatorProjection());
    m_assets = _other.assets();
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

int Scene::addJsFunction(const std::string& _function) {
    for (size_t i = 0; i < m_jsFunctions.size(); i++) {
        if (m_jsFunctions[i] == _function) { return i; }
    }
    m_jsFunctions.push_back(_function);
    return m_jsFunctions.size()-1;
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

std::shared_ptr<TileSource> Scene::getTileSource(int32_t id) {
    auto it = std::find_if(m_tileSources.begin(), m_tileSources.end(),
                           [&](auto& s){ return s->id() == id; });
    if (it != m_tileSources.end()) {
        return *it;
    }
    return nullptr;
}

std::shared_ptr<TileSource> Scene::getTileSource(const std::string& name) {
    auto it = std::find_if(m_tileSources.begin(), m_tileSources.end(),
                           [&](auto& s){ return s->name() == name; });
    if (it != m_tileSources.end()) {
        return *it;
    }
    return nullptr;
}

void Scene::setPixelScale(float _scale) {
    m_pixelScale = _scale;
    for (auto& style : m_styles) {
        style->setPixelScale(_scale);
    }
    m_fontContext->setPixelScale(_scale);
}

void Scene::createSceneAsset(const std::shared_ptr<Platform>& platform, const Url& resolvedUrl,
                                 const Url& relativeUrl, const Url& base) {

    auto& resolvedStr = resolvedUrl.string();
    auto& baseStr = base.string();
    std::shared_ptr<Asset> asset;

    if (m_assets.find(resolvedStr) != m_assets.end()) { return; }

    if ( (Url::getPathExtension(resolvedUrl.path()) == "zip") ){
        if (relativeUrl.hasHttpScheme() || (resolvedUrl.hasHttpScheme() && base.isEmpty())) {
            // Data to be fetched later (and zipHandle created) in network callback
            asset = std::make_shared<ZippedAsset>(resolvedStr);

        } else if (relativeUrl.isAbsolute() || base.isEmpty() || !m_assets[baseStr]->zipHandle()) {
            // load zip asset from File if its absolute or no base or base is not a zip asset
            asset = std::make_shared<ZippedAsset>(resolvedStr, nullptr, platform->bytesFromFile(resolvedStr.c_str()));
        } else {
            auto parentAsset = static_cast<ZippedAsset*>(m_assets[baseStr].get());
            // Parent asset (for base Str) must have been created by now
            assert(parentAsset);
            asset = std::make_shared<ZippedAsset>(resolvedStr, nullptr,
                                                                       parentAsset->readBytesFromAsset(platform, resolvedStr));
        }
    } else {
        const auto& parentAsset = m_assets[baseStr];

        if (relativeUrl.isAbsolute() || (parentAsset && !parentAsset->zipHandle())) {
            // Make sure to first check for cases when the asset does not belong within a zipBundle
            asset = std::make_shared<Asset>(resolvedStr);
        } else if (parentAsset && parentAsset->zipHandle()) {
            // Asset is in zip bundle
            asset = std::make_shared<ZippedAsset>(resolvedStr, parentAsset->zipHandle());
        } else {
            asset = std::make_shared<Asset>(resolvedStr);
        }
    }

    m_assets[resolvedStr] = asset;
}

}
