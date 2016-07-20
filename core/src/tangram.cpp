#include "tangram.h"

#include "platform.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "style/material.h"
#include "style/style.h"
#include "labels/labels.h"
#include "text/fontContext.h"
#include "tile/tileManager.h"
#include "tile/tile.h"
#include "gl/error.h"
#include "gl/shaderProgram.h"
#include "gl/renderState.h"
#include "gl/primitives.h"
#include "util/asyncWorker.h"
#include "util/inputHandler.h"
#include "tile/tileCache.h"
#include "util/fastmap.h"
#include "view/view.h"
#include "data/clientGeoJsonSource.h"
#include "gl.h"
#include "gl/hardware.h"
#include "util/ease.h"
#include "util/jobQueue.h"
#include "debug/textDisplay.h"
#include "debug/frameInfo.h"

#include <cmath>
#include <bitset>

namespace Tangram {

const static size_t MAX_WORKERS = 2;

void Map::setEase(EaseField _f, Ease _e) {
    m_eases[static_cast<size_t>(_f)] = _e;
    requestRender();
}
void Map::clearEase(EaseField _f) {
    static Ease none = {};
    m_eases[static_cast<size_t>(_f)] = none;
}

static std::bitset<8> g_flags = 0;

Map::Map() {

    LOG("initialize");

    // Create view
    m_view = std::make_shared<View>();

    // Create a scene object
    m_scene = std::make_shared<Scene>();

    // Input handler
    m_inputHandler = std::make_unique<InputHandler>(m_view);

    // Instantiate workers
    m_tileWorker = std::make_unique<TileWorker>(MAX_WORKERS);

    // Create a tileManager
    m_tileManager = std::make_unique<TileManager>(*m_tileWorker);

    // Label setup
    m_labels = std::make_unique<Labels>();

    // Create an AsyncWorker
    m_asyncWorker = std::make_unique<AsyncWorker>();

    LOG("finish initialize");

}

Map::~Map() {
    // Explicitly destroy all member objects so that we have a chance
    // to run any resulting jobs sent to the JobQueue.
    m_asyncWorker.reset();
    m_inputHandler.reset();
    m_labels.reset();
    m_view.reset();
    m_nextScene.reset();
    m_tileManager.reset();
    m_tileWorker.reset();
    JobQueue::runJobsForCurrentThread();
}

void Map::setScene(std::shared_ptr<Scene>& _scene) {
    {
        std::lock_guard<std::mutex> lock(m_sceneMutex);
        m_scene = _scene;
    }

    m_scene->setPixelScale(m_view->pixelScale());

    auto& camera = m_scene->camera();
    m_view->setCameraType(camera.type);

    switch (camera.type) {
    case CameraType::perspective:
        m_view->setVanishingPoint(camera.vanishingPoint.x,
                                  camera.vanishingPoint.y);
        if (camera.fovStops) {
            m_view->setFieldOfViewStops(camera.fovStops);
        } else {
            m_view->setFieldOfView(camera.fieldOfView);
        }
        break;
    case CameraType::isometric:
        m_view->setObliqueAxis(camera.obliqueAxis.x,
                               camera.obliqueAxis.y);
        break;
    case CameraType::flat:
        break;
    }

    if (m_scene->useScenePosition) {
        glm::dvec2 projPos = m_view->getMapProjection().LonLatToMeters(m_scene->startPosition);
        m_view->setPosition(projPos.x, projPos.y);
        m_view->setZoom(m_scene->startZoom);
    }

    m_inputHandler->setView(m_view);
    m_tileManager->setDataSources(_scene->dataSources());
    m_tileWorker->setScene(_scene);
    setPixelScale(m_view->pixelScale());

    bool animated = m_scene->animated() == Scene::animate::yes;

    if (m_scene->animated() == Scene::animate::none) {
        for (const auto& style : m_scene->styles()) {
            animated |= style->isAnimated();
        }
    }

    if (animated != isContinuousRendering()) {
        setContinuousRendering(animated);
    }
}

void Map::loadScene(const char* _scenePath, bool _useScenePosition) {
    LOG("Loading scene file: %s", _scenePath);

    // Copy old scene
    auto scene = std::make_shared<Scene>(_scenePath);
    scene->useScenePosition = _useScenePosition;

    if (SceneLoader::loadScene(scene)) {
        setScene(scene);
    }
}

void Map::loadSceneAsync(const char* _scenePath, bool _useScenePosition, MapReady _platformCallback) {
    LOG("Loading scene file (async): %s", _scenePath);

    {
        std::lock_guard<std::mutex> lock(m_sceneMutex);
        m_sceneUpdates.clear();
        m_nextScene = std::make_shared<Scene>(_scenePath);
        m_nextScene->useScenePosition = _useScenePosition;
    }

    JobQueue mainThreadJobQueue;
    mainThreadJobQueue.makeCurrentThreadTarget();

    runAsyncTask([scene = m_nextScene, _platformCallback, mainThreadJobQueue, this](){

            bool ok = SceneLoader::loadScene(scene);

            mainThreadJobQueue.add([scene, ok, _platformCallback, this]() {
                    {
                        std::lock_guard<std::mutex> lock(m_sceneMutex);
                        if (scene == m_nextScene) {
                            m_nextScene.reset();
                        } else { return; }
                    }

                    if (ok) {
                        auto s = scene;
                        setScene(s);
                        applySceneUpdates();
                        if (_platformCallback) { _platformCallback(); }
                    }
                });
        });
}

void Map::queueSceneUpdate(const char* _path, const char* _value) {
    std::lock_guard<std::mutex> lock(m_sceneMutex);
    m_sceneUpdates.push_back({_path, _value});
}

void Map::applySceneUpdates() {

    LOG("Applying %d scene updates", m_sceneUpdates.size());

    if (m_nextScene) {
        // Changes are automatically applied once the scene is loaded
        return;
    }

    std::vector<SceneUpdate> updates;
    {
        std::lock_guard<std::mutex> lock(m_sceneMutex);
        if (m_sceneUpdates.empty()) { return; }

        m_nextScene = std::make_shared<Scene>(*m_scene);
        m_nextScene->useScenePosition = false;

        updates = m_sceneUpdates;
        m_sceneUpdates.clear();
    }

    JobQueue mainThreadJobQueue;
    mainThreadJobQueue.makeCurrentThreadTarget();

    runAsyncTask([scene = m_nextScene, updates = std::move(updates), mainThreadJobQueue, this](){

            SceneLoader::applyUpdates(scene->config(), updates);

            bool ok = SceneLoader::applyConfig(scene->config(), scene);

            mainThreadJobQueue.add([scene, ok, this]() {
                    if (scene == m_nextScene) {
                        std::lock_guard<std::mutex> lock(m_sceneMutex);
                        m_nextScene.reset();
                    } else { return; }

                    if (ok) {
                        auto s = scene;
                        setScene(s);
                        applySceneUpdates();
                    }
                });
        });
}

void Map::resize(int _newWidth, int _newHeight) {

    LOGS("resize: %d x %d", _newWidth, _newHeight);
    LOG("resize: %d x %d", _newWidth, _newHeight);

    glViewport(0, 0, _newWidth, _newHeight);

    if (m_view) {
        m_view->setSize(_newWidth, _newHeight);
    }

    Primitives::setResolution(_newWidth, _newHeight);
}

bool Map::update(float _dt) {

    FrameInfo::beginUpdate();

    JobQueue::runJobsForCurrentThread();

    m_scene->updateTime(_dt);

    bool viewComplete = true;

    for (auto& ease : m_eases) {
        if (!ease.finished()) {
            ease.update(_dt);
            viewComplete = false;
        }
    }

    m_inputHandler->update(_dt);

    m_view->update();

    for (const auto& style : m_scene->styles()) {
        style->onBeginUpdate();
    }

    {
        std::lock_guard<std::mutex> lock(m_tilesMutex);
        ViewState viewState {
            m_view->getMapProjection(),
            m_view->changedOnLastUpdate(),
            glm::dvec2{m_view->getPosition().x, -m_view->getPosition().y },
            m_view->getZoom()
        };

        m_tileManager->updateTileSets(viewState, m_view->getVisibleTiles());

        auto& tiles = m_tileManager->getVisibleTiles();

        if (m_view->changedOnLastUpdate() ||
            m_tileManager->hasTileSetChanged()) {

            for (const auto& tile : tiles) {
                tile->update(_dt, *m_view);
            }
            m_labels->updateLabelSet(*m_view, _dt, m_scene->styles(), tiles,
                                     m_tileManager->getTileCache());

        } else {
            m_labels->updateLabels(*m_view, _dt, m_scene->styles(), tiles);
        }
    }

    FrameInfo::endUpdate();

    bool viewChanged = m_view->changedOnLastUpdate();
    bool tilesChanged = m_tileManager->hasTileSetChanged();
    bool tilesLoading = m_tileManager->hasLoadingTiles();
    bool labelsNeedUpdate = m_labels->needUpdate();
    bool resourceLoading = (m_scene->m_resourceLoad > 0);
    bool nextScene = bool(m_nextScene);

    if (viewChanged || tilesChanged || tilesLoading || labelsNeedUpdate || resourceLoading || nextScene) {
        viewComplete = false;
    }

    // Request for render if labels are in fading in/out states
    if (m_labels->needUpdate()) { requestRender(); }

    return viewComplete;
}

void Map::render() {

    FrameInfo::beginFrame();

    // Invalidate render states for new frame
    if (!m_cacheGlState) {
        RenderState::invalidate();
    }

    // Set up openGL for new frame
    RenderState::depthWrite(GL_TRUE);
    auto& color = m_scene->background();
    RenderState::clearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    for (const auto& style : m_scene->styles()) {
        style->onBeginFrame();
    }

    {
        std::lock_guard<std::mutex> lock(m_tilesMutex);

        // Loop over all styles
        for (const auto& style : m_scene->styles()) {

            style->onBeginDrawFrame(*m_view, *m_scene);

            // Loop over all tiles in m_tileSet
            for (const auto& tile : m_tileManager->getVisibleTiles()) {
                style->draw(*tile);
            }

            style->onEndDrawFrame();
        }
    }

    m_labels->drawDebug(*m_view);

    FrameInfo::draw(*m_view, *m_tileManager);
}

int Map::getViewportHeight() {
    return m_view->getHeight();
}

int Map::getViewportWidth() {
    return m_view->getWidth();
}

float Map::getPixelScale() {
    return m_view->pixelScale();
}

void Map::captureSnapshot(unsigned int* _data) {
    GL_CHECK(glReadPixels(0, 0, m_view->getWidth(), m_view->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)_data));
}

void Map::setPositionNow(double _lon, double _lat) {

    glm::dvec2 meters = m_view->getMapProjection().LonLatToMeters({ _lon, _lat});
    m_view->setPosition(meters.x, meters.y);
    m_inputHandler->cancelFling();
    requestRender();

}

void Map::setPosition(double _lon, double _lat) {

    setPositionNow(_lon, _lat);
    clearEase(EaseField::position);

}

void Map::setPositionEased(double _lon, double _lat, float _duration, EaseType _e) {

    double lon_start, lat_start;
    getPosition(lon_start, lat_start);
    auto cb = [=](float t) { setPositionNow(ease(lon_start, _lon, t, _e), ease(lat_start, _lat, t, _e)); };
    setEase(EaseField::position, { _duration, cb });

}

void Map::getPosition(double& _lon, double& _lat) {

    glm::dvec2 meters(m_view->getPosition().x, m_view->getPosition().y);
    glm::dvec2 degrees = m_view->getMapProjection().MetersToLonLat(meters);
    _lon = degrees.x;
    _lat = degrees.y;

}

void Map::setZoomNow(float _z) {

    m_view->setZoom(_z);
    m_inputHandler->cancelFling();
    requestRender();

}

void Map::setZoom(float _z) {

    setZoomNow(_z);
    clearEase(EaseField::zoom);

}

void Map::setZoomEased(float _z, float _duration, EaseType _e) {

    float z_start = getZoom();
    auto cb = [=](float t) { setZoomNow(ease(z_start, _z, t, _e)); };
    setEase(EaseField::zoom, { _duration, cb });

}

float Map::getZoom() {

    return m_view->getZoom();

}

void Map::setRotationNow(float _radians) {

    m_view->setRoll(_radians);
    requestRender();

}

void Map::setRotation(float _radians) {

    setRotationNow(_radians);
    clearEase(EaseField::rotation);

}

void Map::setRotationEased(float _radians, float _duration, EaseType _e) {

    float radians_start = getRotation();

    // Ease over the smallest angular distance needed
    float radians_delta = glm::mod(_radians - radians_start, (float)TWO_PI);
    if (radians_delta > PI) { radians_delta -= TWO_PI; }
    _radians = radians_start + radians_delta;

    auto cb = [=](float t) { setRotationNow(ease(radians_start, _radians, t, _e)); };
    setEase(EaseField::rotation, { _duration, cb });

}

float Map::getRotation() {

    return m_view->getRoll();

}


void Map::setTiltNow(float _radians) {

    m_view->setPitch(_radians);
    requestRender();

}

void Map::setTilt(float _radians) {

    setTiltNow(_radians);
    clearEase(EaseField::tilt);

}

void Map::setTiltEased(float _radians, float _duration, EaseType _e) {

    float tilt_start = getTilt();
    auto cb = [=](float t) { setTiltNow(ease(tilt_start, _radians, t, _e)); };
    setEase(EaseField::tilt, { _duration, cb });

}

float Map::getTilt() {

    return m_view->getPitch();

}

bool Map::screenPositionToLngLat(double _x, double _y, double* _lng, double* _lat) {

    double intersection = m_view->screenToGroundPlane(_x, _y);
    glm::dvec2 meters(_x + m_view->getPosition().x, _y + m_view->getPosition().y);
    glm::dvec2 lngLat = m_view->getMapProjection().MetersToLonLat(meters);
    *_lng = lngLat.x;
    *_lat = lngLat.y;

    return (intersection >= 0);
}

bool Map::lngLatToScreenPosition(double _lng, double _lat, double* _x, double* _y) {
    bool clipped = false;

    glm::vec2 screenCoords = m_view->lonLatToScreenPosition(_lng, _lat, clipped);

    *_x = screenCoords.x;
    *_y = screenCoords.y;

    bool withinViewport = *_x >= 0. && *_x <= m_view->getWidth() && *_y >= 0. && *_y <= m_view->getHeight();

    return !clipped && withinViewport;
}

void Map::setPixelScale(float _pixelsPerPoint) {

    if (m_view) {
        m_view->setPixelScale(_pixelsPerPoint);
    }

    if (m_scene) {
        m_scene->setPixelScale(_pixelsPerPoint);
    }

    for (auto& style : m_scene->styles()) {
        style->setPixelScale(_pixelsPerPoint);
    }
}

void Map::setCameraType(int _type) {

    m_view->setCameraType(static_cast<CameraType>(_type));
    requestRender();

}

int Map::getCameraType() {

    return static_cast<int>(m_view->cameraType());

}

void Map::addDataSource(std::shared_ptr<DataSource> _source) {
    if (!m_tileManager) { return; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    m_tileManager->addClientDataSource(_source);
}

bool Map::removeDataSource(DataSource& source) {
    if (!m_tileManager) { return false; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    return m_tileManager->removeClientDataSource(source);
}

void Map::clearDataSource(DataSource& _source, bool _data, bool _tiles) {
    if (!m_tileManager) { return; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);

    if (_tiles) { m_tileManager->clearTileSet(_source.id()); }
    if (_data) { _source.clearData(); }

    requestRender();
}

void Map::handleTapGesture(float _posX, float _posY) {

    m_inputHandler->handleTapGesture(_posX, _posY);

}

void Map::handleDoubleTapGesture(float _posX, float _posY) {

    m_inputHandler->handleDoubleTapGesture(_posX, _posY);

}

void Map::handlePanGesture(float _startX, float _startY, float _endX, float _endY) {

    m_inputHandler->handlePanGesture(_startX, _startY, _endX, _endY);

}

void Map::handleFlingGesture(float _posX, float _posY, float _velocityX, float _velocityY) {

    m_inputHandler->handleFlingGesture(_posX, _posY, _velocityX, _velocityY);

}

void Map::handlePinchGesture(float _posX, float _posY, float _scale, float _velocity) {

    m_inputHandler->handlePinchGesture(_posX, _posY, _scale, _velocity);

}

void Map::handleRotateGesture(float _posX, float _posY, float _radians) {

    m_inputHandler->handleRotateGesture(_posX, _posY, _radians);

}

void Map::handleShoveGesture(float _distance) {

    m_inputHandler->handleShoveGesture(_distance);

}

void Map::setupGL() {

    LOG("setup GL");

    if (m_tileManager) {
        m_tileManager->clearTileSets();
    }

    // Reconfigure the render states. Increases context 'generation'.
    // The OpenGL context has been destroyed since the last time resources were
    // created, so we invalidate all data that depends on OpenGL object handles.
    RenderState::increaseGeneration();
    RenderState::invalidate();

    // Set default primitive render color
    Primitives::setColor(0xffffff);

    // Load GL extensions and capabilities
    Hardware::loadExtensions();
    Hardware::loadCapabilities();

    Hardware::printAvailableExtensions();
}

void Map::useCachedGlState(bool _useCache) {
    m_cacheGlState = _useCache;
}

const std::vector<TouchItem>& Map::pickFeaturesAt(float _x, float _y) {
    return m_labels->getFeaturesAtPoint(*m_view, 0, m_scene->styles(),
                                        m_tileManager->getVisibleTiles(),
                                        _x, _y);
}

void Map::runAsyncTask(std::function<void()> _task) {
    m_asyncWorker->enqueue(std::move(_task));
}

void setDebugFlag(DebugFlags _flag, bool _on) {

    g_flags.set(_flag, _on);
    // m_view->setZoom(m_view->getZoom()); // Force the view to refresh

}

bool getDebugFlag(DebugFlags _flag) {

    return g_flags.test(_flag);

}

void toggleDebugFlag(DebugFlags _flag) {

    g_flags.flip(_flag);
    // m_view->setZoom(m_view->getZoom()); // Force the view to refresh

    // Rebuild tiles for debug modes that needs it
    // if (_flag == DebugFlags::proxy_colors
    //  || _flag == DebugFlags::tile_bounds
    //  || _flag == DebugFlags::tile_infos) {
    //     if (m_tileManager) {
    //         std::lock_guard<std::mutex> lock(m_tilesMutex);
    //         m_tileManager->clearTileSets();
    //     }
    // }
}

}
