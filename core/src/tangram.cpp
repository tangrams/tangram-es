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
#include "util/inputHandler.h"
#include "tile/tileCache.h"
#include "util/fastmap.h"
#include "view/view.h"
#include "data/clientGeoJsonSource.h"
#include "gl.h"
#include "gl/hardware.h"
#include "util/ease.h"
#include "debug/textDisplay.h"
#include "debug/frameInfo.h"
#include <atomic>
#include <memory>
#include <array>
#include <cmath>
#include <bitset>
#include <mutex>
#include <queue>
#include <thread>

namespace Tangram {

const static size_t MAX_WORKERS = 2;

std::mutex m_tilesMutex;
std::mutex m_tasksMutex;
std::mutex m_sceneMutex;
std::queue<std::function<void()>> m_tasks;
std::unique_ptr<TileWorker> m_tileWorker;
std::unique_ptr<TileManager> m_tileManager;
std::shared_ptr<Scene> m_scene;
std::shared_ptr<View> m_view;
std::unique_ptr<Labels> m_labels;
std::unique_ptr<InputHandler> m_inputHandler;

std::shared_ptr<Scene> m_nextScene;
std::atomic_bool m_sceneLoading(false);
std::atomic_bool m_sceneUpdating(false);
std::atomic_bool m_newSceneUpdates(false);
std::vector<Scene::Update> m_sceneUpdates;

std::array<Ease, 4> m_eases;
enum class EaseField { position, zoom, rotation, tilt };
void setEase(EaseField _f, Ease _e) {
    m_eases[static_cast<size_t>(_f)] = _e;
    requestRender();
}
void clearEase(EaseField _f) {
    static Ease none = {};
    m_eases[static_cast<size_t>(_f)] = none;
}

static float g_time = 0.0;
static std::bitset<8> g_flags = 0;
static bool g_cacheGlState = false;


class AsyncWorker {
public:

    AsyncWorker() {
        thread = std::thread(&AsyncWorker::run, this);
    }

    ~AsyncWorker() {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_running = false;
        }
        m_condition.notify_all();
        thread.join();
    }

    void enqueue(std::function<void()> _task) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_running) { return; }

            m_queue.push_back(std::move(_task));
        }
        m_condition.notify_one();
    }

private:

    void run() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [&]{ return !m_running || !m_queue.empty(); });
                if (!m_running) { break; }

                task = std::move(m_queue.front());
                m_queue.pop_front();
            }
            task();
        }
    }

    std::thread thread;
    bool m_running = true;
    std::condition_variable m_condition;
    std::mutex m_mutex;
    std::deque<std::function<void()>> m_queue;
};

AsyncWorker m_asyncWorker;

void initialize(const char* _scenePath) {

    if (m_scene && m_scene->path() == _scenePath) {
        LOGD("Specified scene is already initalized.");
        return;
    }

    LOG("initialize");

    // Create view
    m_view = std::make_shared<View>();

    // Create a scene object
    m_scene = std::make_shared<Scene>(_scenePath);

    // Input handler
    m_inputHandler = std::make_unique<InputHandler>(m_view);

    // Instantiate workers
    m_tileWorker = std::make_unique<TileWorker>(MAX_WORKERS);

    // Create a tileManager
    m_tileManager = std::make_unique<TileManager>(*m_tileWorker);

    // Label setup
    m_labels = std::make_unique<Labels>();

    LOG("finish initialize");

}

void setScene(std::shared_ptr<Scene>& _scene) {
    {
        std::lock_guard<std::mutex> lock(m_sceneMutex);
        m_scene = _scene;
    }

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

void loadScene(const char* _scenePath, bool _useScenePosition) {
    LOG("Loading scene file: %s", _scenePath);

    // Copy old scene
    auto scene = std::make_shared<Scene>(_scenePath);

    auto scenePath = setResourceRoot(_scenePath, scene->resourceRoot());
    scene->useScenePosition = _useScenePosition;
    scene->fontContext()->setSceneResourceRoot(scene->resourceRoot());

    if (SceneLoader::loadScene(scenePath, scene)) {
        setScene(scene);
    }
}

void loadSceneAsync(const char* _scenePath, bool _useScenePosition, MapReady _platformCallback) {
    LOG("Loading scene file (async): %s", _scenePath);

    {
        std::lock_guard<std::mutex> lock(m_sceneMutex);
        m_sceneUpdates.clear();
        m_nextScene = std::make_shared<Scene>(_scenePath);
        m_nextScene->useScenePosition = _useScenePosition;
    }

    Tangram::runAsyncTask([scene = m_nextScene, _platformCallback](){

            auto scenePath = setResourceRoot(scene->path().c_str(), scene->resourceRoot());
            scene->fontContext()->setSceneResourceRoot(scene->resourceRoot());

            bool ok = SceneLoader::loadScene(scenePath, scene);

            Tangram::runOnMainLoop([scene, ok, _platformCallback]() {
                    {
                        std::lock_guard<std::mutex> lock(m_sceneMutex);
                        if (scene == m_nextScene) {
                            m_nextScene.reset();
                        } else { return; }
                    }

                    if (ok) {
                        auto s = scene;
                        Tangram::setScene(s);
                        Tangram::applySceneUpdates();
                        if (_platformCallback) { _platformCallback(); }
                    }
                });
        });
}

void queueSceneUpdate(const char* _path, const char* _value) {
    std::lock_guard<std::mutex> lock(m_sceneMutex);
    m_sceneUpdates.push_back({_path, _value});
}

void applySceneUpdates() {

    LOG("Applying %d scene updates", m_sceneUpdates.size());

    if (m_nextScene) {
        // Changes are automatically applied once the scene is loaded
        return;
    }

    std::vector<Scene::Update> updates;
    {
        std::lock_guard<std::mutex> lock(m_sceneMutex);
        if (m_sceneUpdates.empty()) { return; }

        m_nextScene = std::make_shared<Scene>(*m_scene);
        m_nextScene->useScenePosition = false;

        updates = m_sceneUpdates;
        m_sceneUpdates.clear();
    }

    Tangram::runAsyncTask([scene = m_nextScene, updates = std::move(updates)](){

            SceneLoader::applyUpdates(scene->config(), updates);

            bool ok = SceneLoader::applyConfig(scene->config(), *scene);

            Tangram::runOnMainLoop([scene, ok]() {
                    if (scene == m_nextScene) {
                        std::lock_guard<std::mutex> lock(m_sceneMutex);
                        m_nextScene.reset();
                    } else { return; }

                    if (ok) {
                        auto s = scene;
                        Tangram::setScene(s);
                        Tangram::applySceneUpdates();
                    }
                });
        });
}

void resize(int _newWidth, int _newHeight) {

    LOGS("resize: %d x %d", _newWidth, _newHeight);
    LOG("resize: %d x %d", _newWidth, _newHeight);

    glViewport(0, 0, _newWidth, _newHeight);

    if (m_view) {
        m_view->setSize(_newWidth, _newHeight);
    }

    Primitives::setResolution(_newWidth, _newHeight);
}

bool update(float _dt) {

    FrameInfo::beginUpdate();

    g_time += _dt;

    bool viewComplete = true;

    for (auto& ease : m_eases) {
        if (!ease.finished()) {
            ease.update(_dt);
            viewComplete = false;
        }
    }

    size_t nTasks = 0;
    {
        std::lock_guard<std::mutex> lock(m_tasksMutex);
        nTasks = m_tasks.size();
    }
    while (nTasks-- > 0) {
        std::function<void()> task;
        {
            std::lock_guard<std::mutex> lock(m_tasksMutex);
            task = m_tasks.front();
            m_tasks.pop();
        }
        task();
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

    if (viewChanged || tilesChanged || tilesLoading || labelsNeedUpdate) {
        viewComplete = false;
    }

    // Request for render if labels are in fading in/out states
    if (m_labels->needUpdate()) { requestRender(); }

    return viewComplete;
}

void render() {
    FrameInfo::beginFrame();

    // Invalidate render states for new frame
    if (!g_cacheGlState) {
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

void setPositionNow(double _lon, double _lat) {

    glm::dvec2 meters = m_view->getMapProjection().LonLatToMeters({ _lon, _lat});
    m_view->setPosition(meters.x, meters.y);
    m_inputHandler->cancelFling();
    requestRender();

}

void setPosition(double _lon, double _lat) {

    setPositionNow(_lon, _lat);
    clearEase(EaseField::position);

}

void setPosition(double _lon, double _lat, float _duration, EaseType _e) {

    double lon_start, lat_start;
    getPosition(lon_start, lat_start);
    auto cb = [=](float t) { setPositionNow(ease(lon_start, _lon, t, _e), ease(lat_start, _lat, t, _e)); };
    setEase(EaseField::position, { _duration, cb });

}

void getPosition(double& _lon, double& _lat) {

    glm::dvec2 meters(m_view->getPosition().x, m_view->getPosition().y);
    glm::dvec2 degrees = m_view->getMapProjection().MetersToLonLat(meters);
    _lon = degrees.x;
    _lat = degrees.y;

}

void setZoomNow(float _z) {

    m_view->setZoom(_z);
    m_inputHandler->cancelFling();
    requestRender();

}

void setZoom(float _z) {

    setZoomNow(_z);
    clearEase(EaseField::zoom);

}

void setZoom(float _z, float _duration, EaseType _e) {

    float z_start = getZoom();
    auto cb = [=](float t) { setZoomNow(ease(z_start, _z, t, _e)); };
    setEase(EaseField::zoom, { _duration, cb });

}

float getZoom() {

    return m_view->getZoom();

}

void setRotationNow(float _radians) {

    m_view->setRoll(_radians);
    requestRender();

}

void setRotation(float _radians) {

    setRotationNow(_radians);
    clearEase(EaseField::rotation);

}

void setRotation(float _radians, float _duration, EaseType _e) {

    float radians_start = getRotation();

    // Ease over the smallest angular distance needed
    float radians_delta = glm::mod(_radians - radians_start, (float)TWO_PI);
    if (radians_delta > PI) { radians_delta -= TWO_PI; }
    _radians = radians_start + radians_delta;

    auto cb = [=](float t) { setRotationNow(ease(radians_start, _radians, t, _e)); };
    setEase(EaseField::rotation, { _duration, cb });

}

float getRotation() {

    return m_view->getRoll();

}


void setTiltNow(float _radians) {

    m_view->setPitch(_radians);
    requestRender();

}

void setTilt(float _radians) {

    setTiltNow(_radians);
    clearEase(EaseField::tilt);

}

void setTilt(float _radians, float _duration, EaseType _e) {

    float tilt_start = getTilt();
    auto cb = [=](float t) { setTiltNow(ease(tilt_start, _radians, t, _e)); };
    setEase(EaseField::tilt, { _duration, cb });

}

float getTilt() {

    return m_view->getPitch();

}

void screenToWorldCoordinates(double& _x, double& _y) {

    m_view->screenToGroundPlane(_x, _y);
    glm::dvec2 meters(_x + m_view->getPosition().x, _y + m_view->getPosition().y);
    glm::dvec2 lonLat = m_view->getMapProjection().MetersToLonLat(meters);
    _x = lonLat.x;
    _y = lonLat.y;

}

void setPixelScale(float _pixelsPerPoint) {

    if (m_view) {
        m_view->setPixelScale(_pixelsPerPoint);
    }

    for (auto& style : m_scene->styles()) {
        style->setPixelScale(_pixelsPerPoint);
    }
}

void setCameraType(int _type) {

    m_view->setCameraType(static_cast<CameraType>(_type));
    requestRender();

}

int getCameraType() {

    return static_cast<int>(m_view->cameraType());

}

void addDataSource(std::shared_ptr<DataSource> _source) {
    if (!m_tileManager) { return; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    m_tileManager->addClientDataSource(_source);
}

bool removeDataSource(DataSource& source) {
    if (!m_tileManager) { return false; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    return m_tileManager->removeClientDataSource(source);
}

void clearDataSource(DataSource& _source, bool _data, bool _tiles) {
    if (!m_tileManager) { return; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);

    if (_tiles) { m_tileManager->clearTileSet(_source.id()); }
    if (_data) { _source.clearData(); }

    requestRender();
}

void handleTapGesture(float _posX, float _posY) {

    m_inputHandler->handleTapGesture(_posX, _posY);

}

void handleDoubleTapGesture(float _posX, float _posY) {

    m_inputHandler->handleDoubleTapGesture(_posX, _posY);

}

void handlePanGesture(float _startX, float _startY, float _endX, float _endY) {

    m_inputHandler->handlePanGesture(_startX, _startY, _endX, _endY);

}

void handleFlingGesture(float _posX, float _posY, float _velocityX, float _velocityY) {

    m_inputHandler->handleFlingGesture(_posX, _posY, _velocityX, _velocityY);

}

void handlePinchGesture(float _posX, float _posY, float _scale, float _velocity) {

    m_inputHandler->handlePinchGesture(_posX, _posY, _scale, _velocity);

}

void handleRotateGesture(float _posX, float _posY, float _radians) {

    m_inputHandler->handleRotateGesture(_posX, _posY, _radians);

}

void handleShoveGesture(float _distance) {

    m_inputHandler->handleShoveGesture(_distance);

}

void setDebugFlag(DebugFlags _flag, bool _on) {

    g_flags.set(_flag, _on);
    m_view->setZoom(m_view->getZoom()); // Force the view to refresh

}

bool getDebugFlag(DebugFlags _flag) {

    return g_flags.test(_flag);

}

void toggleDebugFlag(DebugFlags _flag) {

    g_flags.flip(_flag);
    m_view->setZoom(m_view->getZoom()); // Force the view to refresh

    // Rebuild tiles for debug modes that needs it
    if (_flag == DebugFlags::proxy_colors
     || _flag == DebugFlags::tile_bounds
     || _flag == DebugFlags::tile_infos) {
        if (m_tileManager) {
            std::lock_guard<std::mutex> lock(m_tilesMutex);
            m_tileManager->clearTileSets();
        }
    }
}

const std::vector<TouchItem>& pickFeaturesAt(float _x, float _y) {
    return m_labels->getFeaturesAtPoint(*m_view, 0, m_scene->styles(),
                                        m_tileManager->getVisibleTiles(),
                                        _x, _y);
}

void useCachedGlState(bool _useCache) {
    g_cacheGlState = _useCache;
}

void setupGL() {

    LOG("setup GL");

    if (m_tileManager) {
        m_tileManager->clearTileSets();
    }

    // Reconfigure the render states. Increases context 'generation'.
    // The OpenGL context has been destroyed since the last time resources were
    // created, so we invalidate all data that depends on OpenGL object handles.
    RenderState::invalidate();
    RenderState::increaseGeneration();

    // Set default primitive render color
    Primitives::setColor(0xffffff);

    // Load GL extensions and capabilities
    Hardware::loadExtensions();
    Hardware::loadCapabilities();

    Hardware::printAvailableExtensions();
}

void runOnMainLoop(std::function<void()> _task) {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_tasks.emplace(std::move(_task));

    requestRender();
}

void runAsyncTask(std::function<void()> _task) {
    m_asyncWorker.enqueue(std::move(_task));
}

float frameTime() {
    return g_time;
}

}
