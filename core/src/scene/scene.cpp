#include "scene/scene.h"

#include "data/tileSource.h"
#include "gl/framebuffer.h"
#include "gl/shaderProgram.h"
#include "labels/labelManager.h"
#include "marker/markerManager.h"
#include "scene/dataLayer.h"
#include "scene/importer.h"
#include "scene/light.h"
#include "scene/sceneLoader.h"
#include "scene/spriteAtlas.h"
#include "scene/stops.h"
#include "selection/featureSelection.h"
#include "selection/selectionQuery.h"
#include "style/material.h"
#include "style/debugStyle.h"
#include "style/debugTextStyle.h"
#include "style/textStyle.h"
#include "style/pointStyle.h"
#include "style/rasterStyle.h"
#include "style/style.h"
#include "text/fontContext.h"
#include "util/base64.h"
#include "util/util.h"
#include "log.h"
#include "scene.h"

#include <algorithm>

namespace Tangram {

static std::atomic<int32_t> s_serial;


Scene::Scene(Platform& _platform, SceneOptions&& _options) :
    id(s_serial++),
    m_platform(_platform),
    m_options(std::move(_options)) {

    m_markerManager = std::make_unique<MarkerManager>(*this);
}

Scene::~Scene() {
    dispose();
}

void Scene::dispose() {
    if (m_state == State::disposed) { return; }
    m_state = State::disposed;

    cancelTasks();

    // TODO: Could check TileSources held by NetworkDatasource urlcallbacks
    // bool waitForTileTasks = true;
    // while(waitForTileTasks) {
    //     waitForTileTasks = false;
    //     for (auto& source : m_tileSources) {
    //         if (source.use_count() > 1) {
    //             waitForTileTasks = true;
    //             break;
    //         }
    //     }
    // }

    if (m_tileManager) {
        /// Cancels all TileTasks
        LOG("Finish TileManager");
        m_tileManager.reset();

        /// Waits for processing TileTasks to finish
        LOG("Finish TileWorker");
        m_tileWorker.reset();
        LOG("TileWorker stopped");
    }
}

void Scene::cancelTasks() {
    auto state = m_state;
    m_state = State::canceled;

    /// NB: Called from main thread - notify async loader thread.
    if (state == State::loading || state == State::pending_resources) {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        /// Cancel loading Scene data
        if (m_importer) {
            LOG("Cancel Importer tasks");
            m_importer->cancelLoading(m_platform);
        }
        /// Cancel pending texture resources
        if (!m_textures.tasks.empty()) {
            LOG("Cancel texture resource tasks");
            for (auto& task : m_textures.tasks) {
                if (task->requestHandle) {
                    m_platform.cancelUrlRequest(task->requestHandle);
                }
            }
        }
        /// Cancel pending font resources
        if (!m_fonts.tasks.empty()) {
            LOG("Cancel font resource tasks");
            for (auto& task : m_fonts.tasks) {
                if (task->requestHandle) {
                    m_platform.cancelUrlRequest(task->requestHandle);
                }
            }
        }
        m_taskCondition.notify_one();
    }

    /// Cancels all TileTasks
    if (m_tileManager) {
        LOG("Cancel TileManager tasks");
        m_tileManager->cancelTileTasks();
    }
}

bool Scene::load() {
    LOGTOInit();
    LOGTO(">>>>>> loadScene >>>>>>");

    auto isCanceled = [&](Scene::State test){
        if (m_state == test) { return false; }
        LOG("Scene got Canceled: %d %d", m_state, test);
        m_errors.emplace_back(SceneError{{}, Error::no_valid_scene});
        return true;
    };

    if (isCanceled(State::initial)) { return false; }

    m_state = State::loading;

    /// Wait until all scene-yamls are available and merged.
    /// NB: Importer holds reference zip archives for resource loading
    ///
    /// Importer is blocking until all imports are loaded
    m_importer = std::make_unique<Importer>();
    m_config = m_importer->loadSceneData(m_platform, m_options.url, m_options.yaml);
    LOGTO("<<< applyImports");

    if (isCanceled(State::loading)) { return false; }

    if (!m_config) {
        LOGE("Scene loading failed: No config!");
        m_errors.emplace_back(SceneError{{}, Error::no_valid_scene});
        return false;
    }

    auto result = SceneLoader::applyUpdates(m_config, m_options.updates);
    if (result.error != Error::none) {
        m_errors.push_back(result);
        LOGE("Applying SceneUpdates failed!");
        return false;
    }
    LOGTO("<<< applyUpdates");

    Importer::resolveSceneUrls(m_config, m_options.url);

    SceneLoader::applyGlobals(m_config);
    LOGTO("<<< applyGlobals");

    m_tileSources = SceneLoader::applySources(m_config, m_options, m_platform);
    LOGTO("<<< applySources");

    SceneLoader::applyCameras(m_config, m_camera);
    LOGTO("<<< applyCameras");

    SceneLoader::applyScene(m_config["scene"], m_background, m_backgroundStops, m_animated);
    LOGTO("<<< applyScene");

    m_tileWorker = std::make_unique<TileWorker>(m_platform, m_options.numTileWorkers);
    m_tileManager = std::make_unique<TileManager>(m_platform, *m_tileWorker);
    m_tileManager->setTileSources(m_tileSources);

    /// Scene is ready to load tiles for initial view
    if (m_options.asyncCallback) {
        m_options.asyncCallback(this);
    }

    SceneLoader::applyTextures(m_config["textures"], m_textures);
    runTextureTasks();
    LOGTO("<<< textures");

    m_fontContext = std::make_unique<FontContext>(m_platform);
    m_fontContext->loadFonts();
    LOGTO("<<< initFonts");

    SceneLoader::applyFonts(m_config["fonts"], m_fonts);
    runFontTasks();
    LOGTO("<<< applyFonts");

    m_styles = SceneLoader::applyStyles(m_config["styles"], m_textures,
                                        m_jsFunctions, m_stops, m_names);
    if (m_options.debugStyles) {
        m_styles.emplace_back(new DebugTextStyle("debugtext", true));
        m_styles.emplace_back(new DebugStyle("debug"));
    }
    /// Styles that are opaque must be ordered first in the scene so that
    /// they are rendered 'under' styles that require blending
    std::sort(m_styles.begin(), m_styles.end(), Style::compare);

    /// Post style sorting set their respective IDs=>vector indices
    /// These indices are used for style geometry lookup in tiles
    for(uint32_t i = 0; i < m_styles.size(); i++) {
        m_styles[i]->setID(i);
        if (auto pointStyle = dynamic_cast<PointStyle*>(m_styles[i].get())) {
            pointStyle->setTextures(m_textures.textures);
            pointStyle->setFontContext(*m_fontContext);
        }
        if (auto textStyle = dynamic_cast<TextStyle*>(m_styles[i].get())) {
            textStyle->setFontContext(*m_fontContext);
        }
    }
    runTextureTasks();
    LOGTO("<<< applyStyles");

    m_lights = SceneLoader::applyLights(m_config["lights"]);
    m_lightShaderBlocks = Light::assembleLights(m_lights);
    LOGTO("<<< applyLights");

    m_layers = SceneLoader::applyLayers(m_config["layers"], m_jsFunctions, m_stops, m_names);
    LOGTO("<<< applyLayers");

    for (auto& style : m_styles) { style->build(*this); }
    LOGTO("<<< buildStyles");

    if (isCanceled(State::loading)) { return false; }

    /// Now we are only waiting for pending fonts and textures:
    /// Let's initialize the TileBuilders on TileWorker threads
    /// in the meantime.
    m_tileWorker->setScene(*this);

    m_featureSelection = std::make_unique<FeatureSelection>();
    m_labelManager = std::make_unique<LabelManager>();

    m_state = State::pending_resources;
    while (true) {
        std::unique_lock<std::mutex> lock(m_taskMutex);

        /// Check if scene-loading was canceled
        if (m_state != State::pending_resources) { break; }

        m_textures.tasks.remove_if([&](auto& task) {
           return task->done;
        });

        m_fonts.tasks.remove_if([&](auto& task) {
            if (!task->done) { return false; }
            if (task->response.error) {
                LOGE("Error retrieving font '%s' at %s: ",
                     task->ft.uri.c_str(), task->response.error);
                return true;
            }
            auto&& data = task->response.content;
            m_fontContext->addFont(task->ft, alfons::InputSource(std::move(data)));
            return true;
        });

        /// All done?
        if (m_textures.tasks.empty() && m_fonts.tasks.empty()) { break; }

        LOGTO("Waiting for fonts and textures");
        m_taskCondition.wait(lock);
    }

    /// We got everything needed from Importer
    m_importer.reset();

    if (isCanceled(State::pending_resources)) { return false; }

    if (m_state == State::pending_resources) {
        m_state = State::pending_completion;
    }

    LOGTO("<<<<<< loadScene <<<<<<");
    return true;
}

void Scene::prefetchTiles(const View& _view) {
    View view = _view;

    view.setCamera(m_camera);

    if (m_options.useScenePosition) {
        view.setPosition(m_camera.startPosition);
    }

    if (m_options.prefetchTiles) {
        LOGTO(">>> loadTiles");
        LOG("Prefetch tiles for View: %fx%f / zoom:%f lon:%f lat:%f",
            view.getWidth(), view.getHeight(), view.getZoom(),
            view.getCenterCoordinates().longitude,
            view.getCenterCoordinates().latitude);
        view.update();
        m_tileManager->updateTileSets(view);
        LOGTO("<<< loadTiles");
    }
}

bool Scene::completeScene(View& _view) {
    if (m_state == State::ready) { return true; }
    if (m_state != State::pending_completion) { return false; }

    _view.setCamera(m_camera);

    if (m_options.useScenePosition) {
        _view.setPosition(m_camera.startPosition);
    }

    m_pixelScale = _view.pixelScale();
    m_fontContext->setPixelScale(m_pixelScale);

    for (auto& style : m_styles) {
        style->setPixelScale(m_pixelScale);
    }

    bool animated = m_animated == Scene::animate::yes;
    if (animated != m_platform.isContinuousRendering()) {
        m_platform.setContinuousRendering(animated);
    }

    m_state = State::ready;

    /// Tell TileWorker that Scene is ready, so it can check its work-queue
    m_tileWorker->poke();

    return true;
}

void Scene::setPixelScale(float _scale) {
    if (m_pixelScale == _scale) { return; }
    m_pixelScale = _scale;

    if (m_state != State::ready) {
        /// We update styles pixel scale in 'complete()'.
        /// No need to clear TileSets at this point.
        return;
    }

    for (auto& style : m_styles) {
        style->setPixelScale(_scale);
    }
    m_fontContext->setPixelScale(_scale);

    /// Tiles must be rebuilt to apply the new pixel scale to labels.
    m_tileManager->clearTileSets();

    /// Markers must be rebuilt to apply the new pixel scale.
    m_markerManager->rebuildAll();
}

std::shared_ptr<Texture> SceneTextures::add(const std::string& _name, const Url& _url,
                                            const TextureOptions& _options,
                                            std::unique_ptr<SpriteAtlas> _atlas) {

    std::shared_ptr<Texture> texture = std::make_shared<Texture>(_options);
    textures.emplace(_name, texture);

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
    tasks.push_front(std::make_shared<SceneTextures::Task>(_url, texture));

    return texture;
}

std::shared_ptr<Texture> SceneTextures::get(const std::string& _name) {
    auto entry = textures.find(_name);
    if (entry != textures.end()) {
        return entry->second;
    }
    /// If texture could not be found by name then interpret name as URL
    TextureOptions options;
    return add(_name, _name, options);
}

void Scene::runTextureTasks() {
    std::lock_guard<std::mutex> lock(m_taskMutex);

    for (auto& task : m_textures.tasks) {
        /// Check if the task is already started..
        if (task->condition) { continue; }
        task->condition = &m_taskCondition;

        LOGTInit();
        LOGT("Fetch texture %s", task->url.string().c_str());

        auto cb = [=, t = std::weak_ptr<SceneTextures::Task>(task)](UrlResponse&& response) mutable {
            auto task = t.lock();
            if (!task) { return; }
            LOGT("Received texture %s", task->url.string().c_str());
            if (response.error) {
                LOGE("Error retrieving URL '%s': %s", task->url.string().c_str(), response.error);
            } else {
                /// Decode texture on download thread.
                auto data = reinterpret_cast<const uint8_t*>(response.content.data());
                auto& texture = task->texture;
                if (!texture->loadImageFromMemory(data, response.content.size())) {
                    LOGE("Invalid texture data from URL '%s'", task->url.string().c_str());
                }
                if (auto& sprites = texture->spriteAtlas()) {
                    sprites->updateSpriteNodes({texture->width(), texture->height()});
                }
            }
            task->done = true;
            task->condition->notify_one();
        };

        if (task->url.scheme() == "zip") {
            m_importer->readFromZip(task->url, cb);
        } else {
            task->requestHandle = m_platform.startUrlRequest(task->url, std::move(cb));
        }
    }
}

void SceneFonts::add(const std::string& _uri, const std::string& _family,
                     const std::string& _style, const std::string& _weight) {

    std::string familyNormalized, styleNormalized;
    familyNormalized.resize(_family.size());
    styleNormalized.resize(_style.size());

    std::transform(_family.begin(), _family.end(), familyNormalized.begin(), ::tolower);
    std::transform(_style.begin(), _style.end(), styleNormalized.begin(), ::tolower);
    auto desc = FontDescription{ familyNormalized, styleNormalized, _weight, _uri};

    tasks.push_front(std::make_shared<Task>(Url(_uri), desc));
}

void Scene::runFontTasks() {
    std::lock_guard<std::mutex> lock(m_taskMutex);

    for (auto& task : m_fonts.tasks) {
        /// Check if the task is already started..
        if (task->condition) { continue; }
        task->condition = &m_taskCondition;

        LOGTInit();
        LOGT("Fetch font %s", task->ft.uri.c_str());

        auto cb = [=,t = std::weak_ptr<SceneFonts::Task>(task)](UrlResponse&& response) mutable {
             auto task = t.lock();
             if (!task) { return; }
             LOGT("Received font: %s", task->ft.uri.c_str());
             task->response = std::move(response);
             task->done = true;
             task->condition->notify_one();
        };

        if (task->url.scheme() == "zip") {
            m_importer->readFromZip(task->url, cb);
        } else {
            task->requestHandle = m_platform.startUrlRequest(task->url, std::move(cb));
        }
    }
}

std::tuple<bool,bool,bool> Scene::update(const View& _view, float _dt) {

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

    return { m_tileManager->hasLoadingTiles(), m_labelManager->needUpdate(), markersChanged };
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
    /// Resolve feature selection queries
    for (const auto& selectionQuery : _selectionQueries) {
        selectionQuery.process(_view, _selectionBuffer,
                               *m_markerManager, *m_tileManager,
                               *m_labelManager, colorCache);
    }
}

std::shared_ptr<Texture> Scene::getTexture(const std::string& textureName) const {
    auto texIt = m_textures.textures.find(textureName);
    if (texIt == m_textures.textures.end()) {
        return nullptr;
    }
    return texIt->second;
}

std::shared_ptr<TileSource> Scene::getTileSource(int32_t id) const {
    auto it = std::find_if(m_tileSources.begin(), m_tileSources.end(),
                           [&](auto& s){ return s->id() == id; });
    if (it != m_tileSources.end()) {
        return *it;
    }
    return nullptr;
}

Color Scene::backgroundColor(int _zoom) const {
    if (m_backgroundStops.frames.size() > 0) {
        return m_backgroundStops.evalColor(_zoom);
    }
    return m_background;
}

int SceneFunctions::add(const std::string& _function) {
    for (size_t i = 0; i <size(); i++) {
        if (at(i) == _function) { return i; }
    }
    push_back(_function);
    return size()-1;
}

}
