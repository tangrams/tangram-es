#include "scene/scene.h"

#include "data/tileSource.h"
#include "gl/framebuffer.h"
#include "gl/shaderProgram.h"
#include "labels/labelManager.h"
#include "scene/dataLayer.h"
#include "scene/importer.h"
#include "scene/light.h"
#include "scene/sceneLoader.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "selection/featureSelection.h"
#include "selection/selectionQuery.h"
#include "style/material.h"
#include "style/style.h"
#include "text/fontContext.h"
#include "util/base64.h"
#include "util/mapProjection.h"
#include "util/util.h"
#include "util/zipArchive.h"
#include "log.h"
#include "scene.h"

#include <algorithm>

namespace Tangram {

static std::atomic<int32_t> s_serial;


Scene::Scene(Platform& _platform) : id(s_serial++), m_platform(_platform) {}

Scene::~Scene() {
    m_tileWorker.reset();
}


bool Scene::load(SceneOptions&& _sceneOptions) {

    if (m_loading || m_ready) { return false; }

    m_options = std::move(_sceneOptions);
    m_view = std::make_unique<View>();
    m_view->setSize(_sceneOptions.view.width, _sceneOptions.view.height);

    m_pixelScale  = _sceneOptions.view.pixelScale;
    m_view->setPixelScale(m_pixelScale);

    LOGTOInit();
    LOGTO(">>>>>> loadScene >>>>>>");
    m_loading = true;

    m_importer = std::make_unique<Importer>();

    // Wait until all scene-yamls are available and merged.
    // Importer also holds reference zip archives
    m_config = m_importer->loadSceneData(m_platform, m_options.url, m_options.yaml);
    LOGTO("<<< applyImports AKA load files, parse YAMLS, allocate Document and merge stuff");

    if (!m_config) {
        LOGE("Scene loading failed: No config!");
        return false;
    }

    LOGTO(">>> applyUpdates");
    if (!SceneLoader::applyUpdates(*this, m_options.updates)) {
        LOGE("Applying SceneUpdates failed!");
        return false;
    }
    LOGTO("<<< applyUpdates");

    LOGTO(">>> applyGlobals");
    SceneLoader::applyGlobals(*this);
    LOGTO("<<< applyGlobals");

    LOGTO(">>> applySources");
    SceneLoader::applySources(*this);
    LOGTO("<<< applySources");

    SceneLoader::applyCameras(*this);
    LOGTO("<<< applyCameras");

    LOGTO(">>> loadTiles");
    // Scene is ready to load tiles for initial view
    if (m_options.prefetchTiles) { initTileManager(); }
    LOGTO("<<< loadTiles");

    LOGTO(">>> textures");
    SceneLoader::applyTextures(*this);
    LOGTO("<<< textures");

    LOGTO(">>> initFonts");
    m_fontContext = std::make_shared<FontContext>(m_platform);
    m_fontContext->loadFonts();
    LOGTO("<<< initFonts");

    LOGTO(">>> applyFonts");
    SceneLoader::applyFonts(*this);
    LOGTO("<<< applyFonts");

    LOGTO(">>> applyStyles");
    SceneLoader::applyStyles(*this);
    LOGTO("<<< applyStyles");

    LOGTO(">>> applyLayers");
    SceneLoader::applyLayers(*this);
    LOGTO("<<< applyLayers");

    LOGTO(">>> applyLights");
    SceneLoader::applyLights(*this);
    LOGTO("<<< applyLights");

    LOGTO(">>> applyScene");
    SceneLoader::applyScene(*this);
    LOGTO("<<< applyScene");

    LOGTO(">>> buildStyles");
    for (auto& style : m_styles) { style->build(*this); }
    LOGTO("<<< buildStyles");

    // Now only waiting for pending fonts and textures:
    // Let the TileWorker initialize its TileBuilders
    if (m_options.prefetchTiles) { startTileWorker(); }

    m_featureSelection = std::make_unique<FeatureSelection>();
    m_labelManager = std::make_unique<LabelManager>();

    LOGTO("<<<<<< loadScene <<<<<<");

    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        for (auto& task : m_pendingTextures) {
            if (task->cb) { m_importer->readFromZip(task->url, task->cb); }
        }
        for (auto& task : m_pendingFonts) {
            if (task->cb) { m_importer->readFromZip(task->url, task->cb); }
        }
    }
    m_importer.reset();

    return true;
}

void Scene::initTileManager() {
    m_tileWorker = std::make_unique<TileWorker>(m_platform, m_options.numTileWorkers);
    m_tileManager = std::make_unique<TileManager>(m_platform, *m_tileWorker);

    m_tileManager->setTileSources(m_tileSources);
    m_tileManager->updateTileSets(*m_view);
}
void Scene::startTileWorker() {
    m_tileWorker->setScene(*this);
}

void Scene::stopTileWorker() {
    m_ready = false;
    m_tileWorker.reset();
}

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
    if (m_pixelScale == _scale) { return; }
    LOGD("setPixelScale %f", _scale);

    m_pixelScale = _scale;

    if (!m_ready) { return; }

    for (auto& style : m_styles) {
        style->setPixelScale(_scale);
    }
    m_fontContext->setPixelScale(_scale);

    // Tiles must be rebuilt to apply the new pixel scale to labels.
    m_tileManager->clearTileSets();

    // Markers must be rebuilt to apply the new pixel scale.
    if (m_markerManager) {
        m_markerManager->rebuildAll();
    }
}

std::shared_ptr<Texture> Scene::fetchTexture(const std::string& _name, const Url& _url,
                                             const TextureOptions& _options,
                                             std::unique_ptr<SpriteAtlas> _atlas) {

    std::shared_ptr<Texture> texture = std::make_shared<Texture>(_options);
    m_textures.emplace(_name, texture);

    if (_url.hasBase64Data() && _url.mediaType() == "image/png") {
        auto data = _url.data();

        std::vector<unsigned char> blob;

        try {
            blob = Base64::decode(data);
        } catch(const std::runtime_error& e) {
            LOGE("Can't decode Base64 texture '%s'", e.what());
        }

        if (blob.empty()) {
            LOGE("Can't decode Base64 texture");

        } else if (!texture->loadImageFromMemory(blob.data(), blob.size())) {
            LOGE("Invalid Base64 texture");
        }
        return texture;
    }

    texture->setSpriteAtlas(std::move(_atlas));

    auto task = std::make_shared<TextureTask>(_url, texture);

    LOGTInit();

    auto cb = [=, t = std::weak_ptr<TextureTask>(task)] (UrlResponse&& response) mutable {
        auto task = t.lock();
        if (!task) { return; }

        LOGT("Received texture %s", task->url.string().c_str());

        if (response.error) {
            LOGE("Error retrieving URL '%s': %s", task->url.string().c_str(), response.error);
        } else {
            auto data = reinterpret_cast<const uint8_t*>(response.content.data());
            auto length = response.content.size();
            if (!task->texture->loadImageFromMemory(data, length)) {
                LOGE("Invalid texture data from URL '%s'", task->url.string().c_str());
            }
            if (auto& sprites = task->texture->spriteAtlas()) {
                sprites->updateSpriteNodes({task->texture->width(),
                                            task->texture->height()});
            }
        }
        task->done = true;
    };

    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_pendingTextures.push_front(task);
        if (_url.scheme() == "zip") {
            task->cb = cb;
        } else {
            LOGT("Fetch    texture %s", task->url.string().c_str());
            m_platform.startUrlRequest(_url, std::move(cb));
        }
    }

    return texture;
}

std::shared_ptr<Texture> Scene::loadTexture(const std::string& _name) {

    auto entry = m_textures.find(_name);
    if (entry != m_textures.end()) {
        return entry->second;
    }

    // If texture could not be found by name then interpret name as URL
    TextureOptions options;
    return fetchTexture(_name, _name, options);
}

void Scene::loadFont(const std::string& _uri, const std::string& _family,
                     const std::string& _style, const std::string& _weight) {

    std::string familyNormalized, styleNormalized;

    familyNormalized.resize(_family.size());
    styleNormalized.resize(_style.size());

    std::transform(_family.begin(), _family.end(), familyNormalized.begin(), ::tolower);
    std::transform(_style.begin(), _style.end(), styleNormalized.begin(), ::tolower);

    Url url(_uri);

    auto task = std::make_shared<FontTask>(url, FontDescription{familyNormalized,styleNormalized,
                                                                _weight, _uri}, m_fontContext);
    LOGTInit();

    auto cb = [=,t = std::weak_ptr<FontTask>(task)] (UrlResponse&& response) mutable {
         auto task = t.lock();
         if (!task) { return; }

         LOGT("Received font: %s", task->ft.uri.c_str());

        if (response.error) {
            LOGE("Error retrieving font '%s' at %s: ", task->ft.alias.c_str(),
                 task->ft.uri.c_str(), response.error);
        } else {
            task->fontContext->addFont(task->ft, alfons::InputSource(std::move(response.content)));
        }
        task->done = true;
    };

    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_pendingFonts.push_front(task);

        if (url.scheme() == "zip") {
            task->cb = cb;
        } else {
            LOGT("Fetch    font %s", task->ft.uri.c_str());
            m_platform.startUrlRequest(url, std::move(cb));
        }
    }
}

bool Scene::complete() {
    if (m_ready) { return true; }
    if (!m_loading) { return false; }

    // Wait until font and texture resources are fully loaded
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        int t = 0, f = 0;
        m_pendingTextures.remove_if([&](auto& task) { t++;  return task->done; });
        m_pendingFonts.remove_if([&](auto& task) { f++; return task->done; });

        if (!m_pendingTextures.empty() || !m_pendingFonts.empty()) {
            LOGTO("Waiting... fonts:%d textures:%d", f, t);
            return false;
        }
    }

    m_ready = true;
    m_loading = false;

    for (auto& style : m_styles) {
        style->setPixelScale(m_pixelScale);
    }
    m_fontContext->setPixelScale(m_pixelScale);

    // Tell the TileWorker Scene is ready so it should check its work-queue
    m_tileWorker->poke();

    m_markerManager = std::make_unique<MarkerManager>(*this);

    return true;
}

bool Scene::update(const View& _view, float _dt) {

    m_time += _dt;

    bool markersChanged = m_markerManager->update(_view, _dt);

    for (const auto& style : m_styles) {
        style->onBeginUpdate();
    }

    m_tileManager->updateTileSets(_view);

    auto& tiles = m_tileManager->getVisibleTiles();
    auto& markers = m_markerManager->markers();

    if (_view.changedOnLastUpdate() ||
        m_tileManager->hasTileSetChanged() ||
        markersChanged) {

        for (const auto& tile : tiles) {
            tile->update(_dt, _view);
        }
        m_labelManager->updateLabelSet(_view.state(), _dt, *this, tiles, markers,
                                       *m_tileManager);
    } else {
        m_labelManager->updateLabels(_view.state(), _dt, m_styles, tiles, markers);
    }

    bool tilesChanged = m_tileManager->hasTileSetChanged();
    bool tilesLoading = m_tileManager->hasLoadingTiles();

    return tilesChanged || tilesLoading;
}

void Scene::renderBeginFrame(RenderState& _rs) {
    for (const auto& style : m_styles) {
        style->onBeginFrame(_rs);
    }
}

bool Scene::render(RenderState& _rs, View& _view) {

    bool drawnAnimatedStyle = false;
    for (const auto& style : m_styles) {

        bool styleDrawn = style->draw(_rs, _view, *this,
                                      m_tileManager->getVisibleTiles(),
                                      m_markerManager->markers());

        drawnAnimatedStyle |= (styleDrawn && style->isAnimated());
    }
    return drawnAnimatedStyle;
}

void Scene::renderSelection(RenderState& _rs, View& _view, FrameBuffer& _selectionBuffer,
                            std::vector<SelectionQuery>& _selectionQueries) {

    for (const auto& style : m_styles) {

        style->drawSelectionFrame(_rs, _view, *this,
                                  m_tileManager->getVisibleTiles(),
                                  m_markerManager->markers());
    }

    std::vector<SelectionColorRead> colorCache;
    // Resolve feature selection queries
    for (const auto& selectionQuery : _selectionQueries) {
        selectionQuery.process(_view, _selectionBuffer,
                               *m_markerManager, *m_tileManager,
                               *m_labelManager, colorCache);
    }

}
}
