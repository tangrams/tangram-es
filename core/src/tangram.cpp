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

enum class EaseField { position, zoom, rotation, tilt };

class Map::Impl {

public:

    void setScene(std::shared_ptr<Scene>& _scene);

    void setEase(EaseField _f, Ease _e);
    void clearEase(EaseField _f);

    void setPositionNow(double _lon, double _lat);
    void setZoomNow(float _z);
    void setRotationNow(float _radians);
    void setTiltNow(float _radians);

    void setPixelScale(float _pixelsPerPoint);

    std::mutex tilesMutex;
    std::mutex sceneMutex;

    View view;
    Labels labels;
    AsyncWorker asyncWorker;
    InputHandler inputHandler{view};
    TileWorker tileWorker{MAX_WORKERS};
    TileManager tileManager{tileWorker};

    std::vector<SceneUpdate> sceneUpdates;
    std::array<Ease, 4> eases;

    std::shared_ptr<Scene> scene = std::make_shared<Scene>();
    std::shared_ptr<Scene> nextScene = nullptr;

    bool cacheGlState;

};

void Map::Impl::setEase(EaseField _f, Ease _e) {
    eases[static_cast<size_t>(_f)] = _e;
    requestRender();
}
void Map::Impl::clearEase(EaseField _f) {
    static Ease none = {};
    eases[static_cast<size_t>(_f)] = none;
}

static std::bitset<8> g_flags = 0;

Map::Map() {

    impl = std::make_unique<Impl>();

}

Map::~Map() {
    // Explicitly destroy all member objects so that we have a chance
    // to run any resulting jobs sent to the JobQueue.
    impl.reset();
    JobQueue::runJobsForCurrentThread();
}

void Map::Impl::setScene(std::shared_ptr<Scene>& _scene) {
    {
        std::lock_guard<std::mutex> lock(sceneMutex);
        scene = _scene;
    }

    scene->setPixelScale(view.pixelScale());

    auto& camera = scene->camera();
    view.setCameraType(camera.type);

    switch (camera.type) {
    case CameraType::perspective:
        view.setVanishingPoint(camera.vanishingPoint.x, camera.vanishingPoint.y);
        if (camera.fovStops) {
            view.setFieldOfViewStops(camera.fovStops);
        } else {
            view.setFieldOfView(camera.fieldOfView);
        }
        break;
    case CameraType::isometric:
        view.setObliqueAxis(camera.obliqueAxis.x, camera.obliqueAxis.y);
        break;
    case CameraType::flat:
        break;
    }

    if (scene->useScenePosition) {
        glm::dvec2 projPos = view.getMapProjection().LonLatToMeters(scene->startPosition);
        view.setPosition(projPos.x, projPos.y);
        view.setZoom(scene->startZoom);
    }

    inputHandler.setView(view);
    tileManager.setDataSources(_scene->dataSources());
    tileWorker.setScene(_scene);
    setPixelScale(view.pixelScale());

    bool animated = scene->animated() == Scene::animate::yes;

    if (scene->animated() == Scene::animate::none) {
        for (const auto& style : scene->styles()) {
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
        impl->setScene(scene);
    }
}

void Map::loadSceneAsync(const char* _scenePath, bool _useScenePosition, MapReady _platformCallback) {
    LOG("Loading scene file (async): %s", _scenePath);

    {
        std::lock_guard<std::mutex> lock(impl->sceneMutex);
        impl->sceneUpdates.clear();
        impl->nextScene = std::make_shared<Scene>(_scenePath);
        impl->nextScene->useScenePosition = _useScenePosition;
    }

    JobQueue mainThreadJobQueue;
    mainThreadJobQueue.makeCurrentThreadTarget();

    runAsyncTask([scene = impl->nextScene, _platformCallback, mainThreadJobQueue, this](){

            bool ok = SceneLoader::loadScene(scene);

            mainThreadJobQueue.add([scene, ok, _platformCallback, this]() {
                    {
                        std::lock_guard<std::mutex> lock(impl->sceneMutex);
                        if (scene == impl->nextScene) {
                            impl->nextScene.reset();
                        } else { return; }
                    }

                    if (ok) {
                        auto s = scene;
                        impl->setScene(s);
                        applySceneUpdates();
                        if (_platformCallback) { _platformCallback(); }
                    }
                });
        });
}

void Map::queueSceneUpdate(const char* _path, const char* _value) {
    std::lock_guard<std::mutex> lock(impl->sceneMutex);
    impl->sceneUpdates.push_back({_path, _value});
}

void Map::applySceneUpdates() {

    LOG("Applying %d scene updates", impl->sceneUpdates.size());

    if (impl->nextScene) {
        // Changes are automatically applied once the scene is loaded
        return;
    }

    std::vector<SceneUpdate> updates;
    {
        std::lock_guard<std::mutex> lock(impl->sceneMutex);
        if (impl->sceneUpdates.empty()) { return; }

        impl->nextScene = std::make_shared<Scene>(*impl->scene);
        impl->nextScene->useScenePosition = false;

        updates = impl->sceneUpdates;
        impl->sceneUpdates.clear();
    }

    JobQueue mainThreadJobQueue;
    mainThreadJobQueue.makeCurrentThreadTarget();

    runAsyncTask([scene = impl->nextScene, updates = std::move(updates), mainThreadJobQueue, this](){

            SceneLoader::applyUpdates(scene->config(), updates);

            bool ok = SceneLoader::applyConfig(scene->config(), scene);

            mainThreadJobQueue.add([scene, ok, this]() {
                    if (scene == impl->nextScene) {
                        std::lock_guard<std::mutex> lock(impl->sceneMutex);
                        impl->nextScene.reset();
                    } else { return; }

                    if (ok) {
                        auto s = scene;
                        impl->setScene(s);
                        applySceneUpdates();
                    }
                });
        });
}

void Map::resize(int _newWidth, int _newHeight) {

    LOGS("resize: %d x %d", _newWidth, _newHeight);
    LOG("resize: %d x %d", _newWidth, _newHeight);

    glViewport(0, 0, _newWidth, _newHeight);

    impl->view.setSize(_newWidth, _newHeight);

    Primitives::setResolution(_newWidth, _newHeight);
}

bool Map::update(float _dt) {

    FrameInfo::beginUpdate();

    JobQueue::runJobsForCurrentThread();

    impl->scene->updateTime(_dt);

    bool viewComplete = true;

    for (auto& ease : impl->eases) {
        if (!ease.finished()) {
            ease.update(_dt);
            viewComplete = false;
        }
    }

    impl->inputHandler.update(_dt);

    impl->view.update();

    for (const auto& style : impl->scene->styles()) {
        style->onBeginUpdate();
    }

    {
        std::lock_guard<std::mutex> lock(impl->tilesMutex);
        ViewState viewState {
            impl->view.getMapProjection(),
            impl->view.changedOnLastUpdate(),
            glm::dvec2{ impl->view.getPosition().x, -impl->view.getPosition().y },
            impl->view.getZoom()
        };

        impl->tileManager.updateTileSets(viewState, impl->view.getVisibleTiles());

        auto& tiles = impl->tileManager.getVisibleTiles();

        if (impl->labels.needUpdate() == Label::EvalUpdate::relayout ||
            impl->view.changedOnLastUpdate() ||
            impl->tileManager.hasTileSetChanged()) {

            for (const auto& tile : tiles) {
                tile->update(_dt, impl->view);
            }

            impl->labels.updateLabelSet(impl->view, _dt, impl->scene->styles(), tiles,
                                        *impl->tileManager.getTileCache());
        } else {
            impl->labels.updateLabels(impl->view, _dt, impl->scene->styles(), tiles);
        }
    }

    FrameInfo::endUpdate();

    bool viewChanged = impl->view.changedOnLastUpdate();
    bool tilesChanged = impl->tileManager.hasTileSetChanged();
    bool tilesLoading = impl->tileManager.hasLoadingTiles();
    bool labelsNeedUpdate = impl->labels.needUpdate() != Label::EvalUpdate::none;
    bool resourceLoading = (impl->scene->m_resourceLoad > 0);
    bool nextScene = bool(impl->nextScene);

    if (viewChanged || tilesChanged || tilesLoading || labelsNeedUpdate || resourceLoading || nextScene) {
        viewComplete = false;
    }

    // Request for render if labels are in fading in/out states
    if (labelsNeedUpdate) { requestRender(); }

    return viewComplete;
}

void Map::render() {

    FrameInfo::beginFrame();

    // Invalidate render states for new frame
    if (!impl->cacheGlState) {
        RenderState::invalidate();
    }

    // Set up openGL for new frame
    RenderState::depthWrite(GL_TRUE);
    auto& color = impl->scene->background();
    RenderState::clearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    for (const auto& style : impl->scene->styles()) {
        style->onBeginFrame();
    }

    {
        std::lock_guard<std::mutex> lock(impl->tilesMutex);

        // Loop over all styles
        for (const auto& style : impl->scene->styles()) {

            style->onBeginDrawFrame(impl->view, *(impl->scene));

            // Loop over all tiles in m_tileSet
            for (const auto& tile : impl->tileManager.getVisibleTiles()) {
                style->draw(*tile);
            }

            style->onEndDrawFrame();
        }
    }

    impl->labels.drawDebug(impl->view);

    FrameInfo::draw(impl->view, impl->tileManager);
}

int Map::getViewportHeight() {
    return impl->view.getHeight();
}

int Map::getViewportWidth() {
    return impl->view.getWidth();
}

float Map::getPixelScale() {
    return impl->view.pixelScale();
}

void Map::captureSnapshot(unsigned int* _data) {
    GL_CHECK(glReadPixels(0, 0, impl->view.getWidth(), impl->view.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)_data));
}

void Map::Impl::setPositionNow(double _lon, double _lat) {

    glm::dvec2 meters = view.getMapProjection().LonLatToMeters({ _lon, _lat});
    view.setPosition(meters.x, meters.y);
    inputHandler.cancelFling();
    requestRender();

}

void Map::setPosition(double _lon, double _lat) {

    impl->setPositionNow(_lon, _lat);
    impl->clearEase(EaseField::position);

}

void Map::setPositionEased(double _lon, double _lat, float _duration, EaseType _e) {

    double lon_start, lat_start;
    getPosition(lon_start, lat_start);
    auto cb = [=](float t) { impl->setPositionNow(ease(lon_start, _lon, t, _e), ease(lat_start, _lat, t, _e)); };
    impl->setEase(EaseField::position, { _duration, cb });

}

void Map::getPosition(double& _lon, double& _lat) {

    glm::dvec2 meters(impl->view.getPosition().x, impl->view.getPosition().y);
    glm::dvec2 degrees = impl->view.getMapProjection().MetersToLonLat(meters);
    _lon = degrees.x;
    _lat = degrees.y;

}

void Map::Impl::setZoomNow(float _z) {

    view.setZoom(_z);
    inputHandler.cancelFling();
    requestRender();

}

void Map::setZoom(float _z) {

    impl->setZoomNow(_z);
    impl->clearEase(EaseField::zoom);

}

void Map::setZoomEased(float _z, float _duration, EaseType _e) {

    float z_start = getZoom();
    auto cb = [=](float t) { impl->setZoomNow(ease(z_start, _z, t, _e)); };
    impl->setEase(EaseField::zoom, { _duration, cb });

}

float Map::getZoom() {

    return impl->view.getZoom();

}

void Map::Impl::setRotationNow(float _radians) {

    view.setRoll(_radians);
    requestRender();

}

void Map::setRotation(float _radians) {

    impl->setRotationNow(_radians);
    impl->clearEase(EaseField::rotation);

}

void Map::setRotationEased(float _radians, float _duration, EaseType _e) {

    float radians_start = getRotation();

    // Ease over the smallest angular distance needed
    float radians_delta = glm::mod(_radians - radians_start, (float)TWO_PI);
    if (radians_delta > PI) { radians_delta -= TWO_PI; }
    _radians = radians_start + radians_delta;

    auto cb = [=](float t) { impl->setRotationNow(ease(radians_start, _radians, t, _e)); };
    impl->setEase(EaseField::rotation, { _duration, cb });

}

float Map::getRotation() {

    return impl->view.getRoll();

}


void Map::Impl::setTiltNow(float _radians) {

    view.setPitch(_radians);
    requestRender();

}

void Map::setTilt(float _radians) {

    impl->setTiltNow(_radians);
    impl->clearEase(EaseField::tilt);

}

void Map::setTiltEased(float _radians, float _duration, EaseType _e) {

    float tilt_start = getTilt();
    auto cb = [=](float t) { impl->setTiltNow(ease(tilt_start, _radians, t, _e)); };
    impl->setEase(EaseField::tilt, { _duration, cb });

}

float Map::getTilt() {

    return impl->view.getPitch();

}

bool Map::screenPositionToLngLat(double _x, double _y, double* _lng, double* _lat) {

    double intersection = impl->view.screenToGroundPlane(_x, _y);
    glm::dvec3 eye = impl->view.getPosition();
    glm::dvec2 meters(_x + eye.x, _y + eye.y);
    glm::dvec2 lngLat = impl->view.getMapProjection().MetersToLonLat(meters);
    *_lng = lngLat.x;
    *_lat = lngLat.y;

    return (intersection >= 0);
}

bool Map::lngLatToScreenPosition(double _lng, double _lat, double* _x, double* _y) {
    bool clipped = false;

    glm::vec2 screenCoords = impl->view.lonLatToScreenPosition(_lng, _lat, clipped);

    *_x = screenCoords.x;
    *_y = screenCoords.y;

    float width = impl->view.getWidth();
    float height = impl->view.getHeight();
    bool withinViewport = *_x >= 0. && *_x <= width && *_y >= 0. && *_y <= height;

    return !clipped && withinViewport;
}

void Map::setPixelScale(float _pixelsPerPoint) {

    impl->setPixelScale(_pixelsPerPoint);

}

void Map::Impl::setPixelScale(float _pixelsPerPoint) {

    view.setPixelScale(_pixelsPerPoint);
    scene->setPixelScale(_pixelsPerPoint);
    for (auto& style : scene->styles()) {
        style->setPixelScale(_pixelsPerPoint);
    }

}

void Map::setCameraType(int _type) {

    impl->view.setCameraType(static_cast<CameraType>(_type));
    requestRender();

}

int Map::getCameraType() {

    return static_cast<int>(impl->view.cameraType());

}

void Map::addDataSource(std::shared_ptr<DataSource> _source) {
    std::lock_guard<std::mutex> lock(impl->tilesMutex);
    impl->tileManager.addClientDataSource(_source);
}

bool Map::removeDataSource(DataSource& source) {
    std::lock_guard<std::mutex> lock(impl->tilesMutex);
    return impl->tileManager.removeClientDataSource(source);
}

void Map::clearDataSource(DataSource& _source, bool _data, bool _tiles) {
    std::lock_guard<std::mutex> lock(impl->tilesMutex);

    if (_tiles) { impl->tileManager.clearTileSet(_source.id()); }
    if (_data) { _source.clearData(); }

    requestRender();
}

void Map::handleTapGesture(float _posX, float _posY) {

    impl->inputHandler.handleTapGesture(_posX, _posY);

}

void Map::handleDoubleTapGesture(float _posX, float _posY) {

    impl->inputHandler.handleDoubleTapGesture(_posX, _posY);

}

void Map::handlePanGesture(float _startX, float _startY, float _endX, float _endY) {

    impl->inputHandler.handlePanGesture(_startX, _startY, _endX, _endY);

}

void Map::handleFlingGesture(float _posX, float _posY, float _velocityX, float _velocityY) {

    impl->inputHandler.handleFlingGesture(_posX, _posY, _velocityX, _velocityY);

}

void Map::handlePinchGesture(float _posX, float _posY, float _scale, float _velocity) {

    impl->inputHandler.handlePinchGesture(_posX, _posY, _scale, _velocity);

}

void Map::handleRotateGesture(float _posX, float _posY, float _radians) {

    impl->inputHandler.handleRotateGesture(_posX, _posY, _radians);

}

void Map::handleShoveGesture(float _distance) {

    impl->inputHandler.handleShoveGesture(_distance);

}

void Map::setupGL() {

    LOG("setup GL");

    impl->tileManager.clearTileSets();

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
    impl->cacheGlState = _useCache;
}

const std::vector<TouchItem>& Map::pickFeaturesAt(float _x, float _y) {
    return impl->labels.getFeaturesAtPoint(impl->view, 0, impl->scene->styles(),
                                           impl->tileManager.getVisibleTiles(),
                                           _x, _y);
}

void Map::runAsyncTask(std::function<void()> _task) {
    impl->asyncWorker.enqueue(std::move(_task));
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
    //  || _flag == DebugFlags::draw_all_labels
    //  || _flag == DebugFlags::tile_bounds
    //  || _flag == DebugFlags::tile_infos) {
    //     if (m_tileManager) {
    //         std::lock_guard<std::mutex> lock(m_tilesMutex);
    //         m_tileManager->clearTileSets();
    //     }
    // }
}

}
