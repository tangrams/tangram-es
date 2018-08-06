#include "map.h"

#include "data/clientGeoJsonSource.h"
#include "debug/textDisplay.h"
#include "debug/frameInfo.h"
#include "gl.h"
#include "gl/glError.h"
#include "gl/framebuffer.h"
#include "gl/hardware.h"
#include "gl/primitives.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "labels/labels.h"
#include "marker/marker.h"
#include "marker/markerManager.h"
#include "platform.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "selection/selectionQuery.h"
#include "style/material.h"
#include "style/style.h"
#include "text/fontContext.h"
#include "tile/tile.h"
#include "tile/tileCache.h"
#include "tile/tileManager.h"
#include "util/asyncWorker.h"
#include "util/fastmap.h"
#include "util/inputHandler.h"
#include "util/ease.h"
#include "util/jobQueue.h"
#include "view/flyTo.h"
#include "view/view.h"

#include <bitset>
#include <cmath>

namespace Tangram {

const static size_t MAX_WORKERS = 2;

struct CameraEase {
    struct {
        glm::dvec2 pos;
        float zoom;
        float rotation;
        float tilt;
    } start, end;
};

using CameraAnimator = std::function<uint32_t(float dt)>;

class Map::Impl {

public:
    Impl(std::shared_ptr<Platform> _platform) :
        platform(_platform),
        inputHandler(_platform, view),
        scene(std::make_shared<Scene>(_platform, Url())),
        tileWorker(_platform, MAX_WORKERS),
        tileManager(_platform, tileWorker) {}

    void setScene(std::shared_ptr<Scene>& _scene);

    void setPixelScale(float _pixelsPerPoint);

    std::mutex tilesMutex;
    std::mutex sceneMutex;

    RenderState renderState;
    JobQueue jobQueue;
    View view;
    Labels labels;
    std::unique_ptr<AsyncWorker> asyncWorker = std::make_unique<AsyncWorker>();
    std::shared_ptr<Platform> platform;
    InputHandler inputHandler;

    std::vector<Ease> eases;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<Scene> lastValidScene;
    std::atomic<int32_t> sceneLoadTasks{0};
    std::condition_variable sceneLoadCondition;

    // NB: Destruction of (managed and loading) tiles must happen
    // before implicit destruction of 'scene' above!
    // In particular any references of Labels and Markers to FontContext
    TileWorker tileWorker;
    TileManager tileManager;
    MarkerManager markerManager;
    std::unique_ptr<FrameBuffer> selectionBuffer = std::make_unique<FrameBuffer>(0, 0);

    bool cacheGlState = false;
    float pickRadius = .5f;

    std::vector<SelectionQuery> selectionQueries;

    SceneReadyCallback onSceneReady = nullptr;
    CameraAnimationCallback cameraAnimationListener = nullptr;

    void sceneLoadBegin() {
        sceneLoadTasks++;
    }

    void sceneLoadEnd() {
        sceneLoadTasks--;
        assert(sceneLoadTasks >= 0);

        sceneLoadCondition.notify_one();
    }

};


static std::bitset<9> g_flags = 0;

Map::Map(std::shared_ptr<Platform> _platform) : platform(_platform) {
    impl.reset(new Impl(_platform));
}

Map::~Map() {
    // The unique_ptr to Impl will be automatically destroyed when Map is destroyed.
    impl->tileWorker.stop();
    impl->asyncWorker.reset();

    // Make sure other threads are stopped before calling stop()!
    // All jobs will be executed immediately on add() afterwards.
    impl->jobQueue.stop();

    TextDisplay::Instance().deinit();
    Primitives::deinit();
}

void Map::Impl::setScene(std::shared_ptr<Scene>& _scene) {

    scene = _scene;

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

    if (camera.maxTiltStops) {
        view.setMaxPitchStops(camera.maxTiltStops);
    } else {
        view.setMaxPitch(camera.maxTilt);
    }

    if (scene->useScenePosition) {
        glm::dvec2 projPos = view.getMapProjection().LonLatToMeters(scene->startPosition);
        view.setPosition(projPos.x, projPos.y);
        view.setZoom(scene->startZoom);
    }

    inputHandler.setView(view);
    tileManager.setTileSources(_scene->tileSources());
    tileWorker.setScene(_scene);
    markerManager.setScene(_scene);

    bool animated = scene->animated() == Scene::animate::yes;

    if (scene->animated() == Scene::animate::none) {
        for (const auto& style : scene->styles()) {
            animated |= style->isAnimated();
        }
    }

    if (animated != platform->isContinuousRendering()) {
        platform->setContinuousRendering(animated);
    }
}

// NB: Not thread-safe. Must be called on the main/render thread!
// (Or externally synchronized with main/render thread)
SceneID Map::loadScene(std::shared_ptr<Scene> scene,
                       const std::vector<SceneUpdate>& _sceneUpdates) {

    {
        std::unique_lock<std::mutex> lock(impl->sceneMutex);

        impl->sceneLoadCondition.wait(lock, [&]{ return impl->sceneLoadTasks == 0; });

        impl->lastValidScene.reset();
    }

    if (SceneLoader::loadScene(platform, scene, _sceneUpdates)) {
        impl->setScene(scene);

        {
            std::lock_guard<std::mutex> lock(impl->sceneMutex);
            impl->lastValidScene = scene;
        }
    }

    if (impl->onSceneReady) {
        if (scene->errors.empty()) {
            impl->onSceneReady(scene->id, nullptr);
        } else {
            impl->onSceneReady(scene->id, &(scene->errors.front()));
        }
    }
    return scene->id;
}

SceneID Map::loadScene(const std::string& _scenePath, bool _useScenePosition,
                       const std::vector<SceneUpdate>& _sceneUpdates) {

    LOG("Loading scene file: %s", _scenePath.c_str());
    auto scene = std::make_shared<Scene>(platform, _scenePath);
    scene->useScenePosition = _useScenePosition;
    return loadScene(scene, _sceneUpdates);
}

SceneID Map::loadSceneYaml(const std::string& _yaml, const std::string& _resourceRoot,
                           bool _useScenePosition, const std::vector<SceneUpdate>& _sceneUpdates) {

    LOG("Loading scene string");
    auto scene = std::make_shared<Scene>(platform, _yaml, _resourceRoot);
    scene->useScenePosition = _useScenePosition;
    return loadScene(scene, _sceneUpdates);
}

SceneID Map::loadSceneAsync(const std::string& _scenePath, bool _useScenePosition,
                            const std::vector<SceneUpdate>& _sceneUpdates) {

    LOG("Loading scene file (async): %s", _scenePath.c_str());
    auto scene = std::make_shared<Scene>(platform, _scenePath);
    scene->useScenePosition = _useScenePosition;
    return loadSceneAsync(scene, _sceneUpdates);
}

SceneID Map::loadSceneYamlAsync(const std::string& _yaml, const std::string& _resourceRoot,
                                bool _useScenePosition, const std::vector<SceneUpdate>& _sceneUpdates) {

    LOG("Loading scene string (async)");
    auto scene = std::make_shared<Scene>(platform, _yaml, _resourceRoot);
    scene->useScenePosition = _useScenePosition;
    return loadSceneAsync(scene, _sceneUpdates);
}

SceneID Map::loadSceneAsync(std::shared_ptr<Scene> nextScene,
                            const std::vector<SceneUpdate>& _sceneUpdates) {

    impl->sceneLoadBegin();

    runAsyncTask([nextScene, _sceneUpdates, this](){

            bool newSceneLoaded = SceneLoader::loadScene(platform, nextScene, _sceneUpdates);
            if (!newSceneLoaded) {

                if (impl->onSceneReady) {
                    SceneError err;
                    if (!nextScene->errors.empty()) { err = nextScene->errors.front(); }
                    impl->onSceneReady(nextScene->id, &err);
                }
                impl->sceneLoadEnd();
                return;
            }

            {
                std::lock_guard<std::mutex> lock(impl->sceneMutex);
                // NB: Need to set the scene on the worker thread so that waiting
                // applyUpdates AsyncTasks can access it to copy the config.
                impl->lastValidScene = nextScene;
            }

            impl->jobQueue.add([nextScene, newSceneLoaded, this]() {
                    if (newSceneLoaded) {
                        auto s = nextScene;
                        impl->setScene(s);
                    }
                    if (impl->onSceneReady) { impl->onSceneReady(nextScene->id, nullptr); }
                });

            impl->sceneLoadEnd();
            platform->requestRender();
        });

    return nextScene->id;
}

void Map::setSceneReadyListener(SceneReadyCallback _onSceneReady) {
    impl->onSceneReady = _onSceneReady;
}

void Map::setCameraAnimationListener(CameraAnimationCallback _cb) {
    impl->cameraAnimationListener = _cb;
}

std::shared_ptr<Platform>& Map::getPlatform() {
    return platform;
}

SceneID Map::updateSceneAsync(const std::vector<SceneUpdate>& _sceneUpdates) {

    impl->sceneLoadBegin();

    std::vector<SceneUpdate> updates = _sceneUpdates;

    auto nextScene = std::make_shared<Scene>();
    nextScene->useScenePosition = false;

    runAsyncTask([nextScene, updates = std::move(updates), this](){

            if (!impl->lastValidScene) {
                if (impl->onSceneReady) {
                    SceneError err {{}, Error::no_valid_scene};
                    impl->onSceneReady(nextScene->id, &err);
                }
                impl->sceneLoadEnd();
                return;
            }

            {
                std::lock_guard<std::mutex> lock(impl->sceneMutex);
                nextScene->copyConfig(*impl->lastValidScene);
            }

            if (!SceneLoader::applyUpdates(platform, *nextScene, updates)) {
                LOGW("Scene updates not applied to current scene");

                if (impl->onSceneReady) {
                    SceneError err;
                    if (!nextScene->errors.empty()) { err = nextScene->errors.front(); }
                    impl->onSceneReady(nextScene->id, &err);
                }
                impl->sceneLoadEnd();
                return;
            }


            bool configApplied = SceneLoader::applyConfig(platform, nextScene);

            {
                std::lock_guard<std::mutex> lock(impl->sceneMutex);
                // NB: Need to set the scene on the worker thread so that waiting
                // applyUpdates AsyncTasks can access it to copy the config.
                if (configApplied) { impl->lastValidScene = nextScene; }
            }
            impl->jobQueue.add([nextScene, configApplied, this]() {

                    if (configApplied) {
                        auto s = nextScene;
                        impl->setScene(s);
                    }
                    if (impl->onSceneReady) { impl->onSceneReady(nextScene->id, nullptr); }
                });

            impl->sceneLoadEnd();
            platform->requestRender();
        });

    return nextScene->id;
}

void Map::setMBTiles(const char* _dataSourceName, const char* _mbtilesFilePath) {
    std::string scenePath = std::string("sources.") + _dataSourceName + ".mbtiles";
    updateSceneAsync({SceneUpdate{scenePath.c_str(), _mbtilesFilePath}});
}

void Map::resize(int _newWidth, int _newHeight) {

    LOGS("resize: %d x %d", _newWidth, _newHeight);
    LOG("resize: %d x %d", _newWidth, _newHeight);

    impl->renderState.viewport(0, 0, _newWidth, _newHeight);

    impl->view.setSize(_newWidth, _newHeight);

    impl->selectionBuffer = std::make_unique<FrameBuffer>(_newWidth/2, _newHeight/2);

    Primitives::setResolution(impl->renderState, _newWidth, _newHeight);
}

bool Map::update(float _dt) {

    impl->jobQueue.runJobs();

    // Wait until font and texture resources are fully loaded
    if (impl->scene->pendingFonts > 0 || impl->scene->pendingTextures > 0) {
        platform->requestRender();
        return false;
    }

    FrameInfo::beginUpdate();

    impl->scene->updateTime(_dt);

    bool viewComplete = true;
    bool markersNeedUpdate = false;
    bool cameraEasing = false;

    if (!impl->eases.empty()) {
        auto& ease = impl->eases[0];
        ease.update(_dt);

        if (ease.finished()) {
            if (impl->cameraAnimationListener) {
                impl->cameraAnimationListener(true);
            }
            impl->eases.clear();
        } else {
            cameraEasing = true;
        }
    }

    impl->inputHandler.update(_dt);

    impl->view.update();

    bool markersChanged = impl->markerManager.update(impl->view, _dt);

    for (const auto& style : impl->scene->styles()) {
        style->onBeginUpdate();
    }

    {
        std::lock_guard<std::mutex> lock(impl->tilesMutex);

        impl->tileManager.updateTileSets(impl->view);

        auto& tiles = impl->tileManager.getVisibleTiles();
        auto& markers = impl->markerManager.markers();

        if (impl->view.changedOnLastUpdate() ||
            impl->tileManager.hasTileSetChanged() ||
            markersChanged) {

            for (const auto& tile : tiles) {
                tile->update(_dt, impl->view);
            }
            impl->labels.updateLabelSet(impl->view.state(), _dt, impl->scene, tiles, markers,
                                        impl->tileManager);
        } else {
            impl->labels.updateLabels(impl->view.state(), _dt, impl->scene->styles(), tiles, markers);
        }
    }

    FrameInfo::endUpdate();

    bool viewChanged = impl->view.changedOnLastUpdate();
    bool tilesChanged = impl->tileManager.hasTileSetChanged();
    bool tilesLoading = impl->tileManager.hasLoadingTiles();
    bool labelsNeedUpdate = impl->labels.needUpdate();

    if (viewChanged || tilesChanged || tilesLoading || labelsNeedUpdate || impl->sceneLoadTasks > 0) {
        viewComplete = false;
    }

    // Request render if labels are in fading states or markers are easing.
    if (cameraEasing || labelsNeedUpdate || markersNeedUpdate) {
        platform->requestRender();
    }

    return viewComplete;
}

void Map::setPickRadius(float _radius) {
    impl->pickRadius = _radius;
}

void Map::pickFeatureAt(float _x, float _y, FeaturePickCallback _onFeaturePickCallback) {
    impl->selectionQueries.push_back({{_x, _y}, impl->pickRadius, _onFeaturePickCallback});

    platform->requestRender();
}

void Map::pickLabelAt(float _x, float _y, LabelPickCallback _onLabelPickCallback) {
    impl->selectionQueries.push_back({{_x, _y}, impl->pickRadius, _onLabelPickCallback});

    platform->requestRender();
}

void Map::pickMarkerAt(float _x, float _y, MarkerPickCallback _onMarkerPickCallback) {
    impl->selectionQueries.push_back({{_x, _y}, impl->pickRadius, _onMarkerPickCallback});

    platform->requestRender();
}

void Map::render() {

    // Do not render if any texture resources are in process of being downloaded
    if (impl->scene->pendingTextures > 0) {
        return;
    }

    bool drawSelectionBuffer = getDebugFlag(DebugFlags::selection_buffer);

    // Cache default framebuffer handle used for rendering
    impl->renderState.cacheDefaultFramebuffer();

    FrameInfo::beginFrame();

    // Invalidate render states for new frame
    if (!impl->cacheGlState) {
        impl->renderState.invalidateStates();
    }

    // Delete batch of gl resources
    impl->renderState.flushResourceDeletion();

    for (const auto& style : impl->scene->styles()) {
        style->onBeginFrame(impl->renderState);
    }

    // Render feature selection pass to offscreen framebuffer
    if (impl->selectionQueries.size() > 0 || drawSelectionBuffer) {
        impl->selectionBuffer->applyAsRenderTarget(impl->renderState);

        std::lock_guard<std::mutex> lock(impl->tilesMutex);

        for (const auto& style : impl->scene->styles()) {

            style->drawSelectionFrame(impl->renderState, impl->view, *(impl->scene),
                                      impl->tileManager.getVisibleTiles(),
                                      impl->markerManager.markers());
        }

        std::vector<SelectionColorRead> colorCache;
        // Resolve feature selection queries
        for (const auto& selectionQuery : impl->selectionQueries) {
            selectionQuery.process(impl->view, *impl->selectionBuffer, impl->markerManager,
                                   impl->tileManager, impl->labels, colorCache);
        }

        impl->selectionQueries.clear();
    }

    // Setup default framebuffer for a new frame
    glm::vec2 viewport(impl->view.getWidth(), impl->view.getHeight());
    FrameBuffer::apply(impl->renderState, impl->renderState.defaultFrameBuffer(),
                       viewport, impl->scene->background().toColorF());

    if (drawSelectionBuffer) {
        impl->selectionBuffer->drawDebug(impl->renderState, viewport);
        FrameInfo::draw(impl->renderState, impl->view, impl->tileManager);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(impl->tilesMutex);

        // Loop over all styles
        for (const auto& style : impl->scene->styles()) {

            style->draw(impl->renderState,
                        impl->view, *(impl->scene),
                        impl->tileManager.getVisibleTiles(),
                        impl->markerManager.markers());

        }
    }

    impl->labels.drawDebug(impl->renderState, impl->view);

    FrameInfo::draw(impl->renderState, impl->view, impl->tileManager);
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
    GL::readPixels(0, 0, impl->view.getWidth(), impl->view.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)_data);
}


CameraPosition Map::getCameraPosition() {
    CameraPosition camera;

    getPosition(camera.longitude, camera.latitude);
    camera.zoom = getZoom();
    camera.rotation = getRotation();
    camera.tilt = getTilt();

    return camera;
}

void Map::cancelCameraAnimation() {
    impl->inputHandler.cancelFling();

    impl->eases.clear();

    if (impl->cameraAnimationListener) {
        impl->cameraAnimationListener(false);
    }
}

void Map::setCameraPosition(const CameraPosition& _camera) {
    cancelCameraAnimation();

    glm::dvec2 meters = impl->view.getMapProjection().LonLatToMeters({ _camera.longitude, _camera.latitude});
    impl->view.setPosition(meters.x, meters.y);
    impl->view.setZoom(_camera.zoom);
    impl->view.setRoll(_camera.rotation);
    impl->view.setPitch(_camera.tilt);

    impl->platform->requestRender();
}

void Map::setCameraPositionEased(const CameraPosition& _camera, float _duration, EaseType _e) {
    cancelCameraAnimation();

    CameraEase e;

    double lonStart, latStart;
    getPosition(lonStart, latStart);

    // if (_camera.longitude != lonStart || _camera.latitude != latStart) {
    double lonEnd = _camera.longitude;
    double latEnd = _camera.latitude;

    double dLongitude = lonEnd - lonStart;
    if (dLongitude > 180.0) {
        lonEnd -= 360.0;
    } else if (dLongitude < -180.0) {
        lonEnd += 360.0;
    }

    e.start.pos = impl->view.getMapProjection().LonLatToMeters({ lonStart, latStart});
    e.end.pos = impl->view.getMapProjection().LonLatToMeters({ lonEnd, latEnd});
    // }

    // if (_camera.zoom != getZoom()) {
    e.start.zoom = getZoom();
    e.end.zoom = _camera.zoom;
    // }

    // if (_camera.rotation != getRotation()) {
    float radiansStart = getRotation();

    // Ease over the smallest angular distance needed
    float radiansDelta = glm::mod(_camera.rotation - radiansStart, (float)TWO_PI);
    if (radiansDelta > PI) { radiansDelta -= TWO_PI; }

    e.start.rotation = radiansStart;
    e.end.rotation = radiansStart + radiansDelta;
    // }

    // if (_camera.tilt != getTilt()) {
    e.start.tilt = getTilt();
    e.end.tilt = _camera.tilt;
    // }

    impl->eases.push_back(Ease{_duration,
        [=](float t) {
            impl->view.setPosition(ease(e.start.pos.x, e.end.pos.x, t, _e),
                                   ease(e.start.pos.y, e.end.pos.y, t, _e));
            impl->view.setZoom(ease(e.start.zoom, e.end.zoom, t, _e));

            impl->view.setRoll(ease(e.start.rotation, e.end.rotation, t, _e));

            impl->view.setPitch(ease(e.start.tilt, e.end.tilt, t, _e));
        }});

    platform->requestRender();
}

void Map::setPosition(double _lon, double _lat) {
    cancelCameraAnimation();

    glm::dvec2 meters = impl->view.getMapProjection().LonLatToMeters({ _lon, _lat});
    impl->view.setPosition(meters.x, meters.y);
    impl->inputHandler.cancelFling();
    impl->platform->requestRender();
}

void Map::getPosition(double& _lon, double& _lat) {
    glm::dvec2 meters(impl->view.getPosition().x, impl->view.getPosition().y);
    glm::dvec2 degrees = impl->view.getMapProjection().MetersToLonLat(meters);
    _lon = LngLat::wrapLongitude(degrees.x);
    _lat = degrees.y;
}

void Map::setZoom(float _z) {
    cancelCameraAnimation();

    impl->view.setZoom(_z);
    impl->inputHandler.cancelFling();
    impl->platform->requestRender();
}

float Map::getZoom() {
    return impl->view.getZoom();
}

void Map::setRotation(float _radians) {
    cancelCameraAnimation();

    impl->view.setRoll(_radians);
    impl->platform->requestRender();
}

float Map::getRotation() {
    return impl->view.getRoll();
}

void Map::setTilt(float _radians) {
    cancelCameraAnimation();

    impl->view.setPitch(_radians);
    impl->platform->requestRender();
}

float Map::getTilt() {
    return impl->view.getPitch();
}

CameraPosition Map::getEnclosingCameraPosition(LngLat _a, LngLat _b, float _buffer) {
    CameraPosition camera;
    const MapProjection& projection = impl->view.getMapProjection();
    glm::dvec2 aMeters = projection.LonLatToMeters(glm::dvec2(_a.longitude, _a.latitude));
    glm::dvec2 bMeters = projection.LonLatToMeters(glm::dvec2(_b.longitude, _b.latitude));
    double distance = glm::distance(aMeters, bMeters) * (1. + _buffer);
    double focusScale = distance / (2. * MapProjection::HALF_CIRCUMFERENCE);
    double viewScale = impl->view.getWidth() / projection.TileSize();
    camera.zoom = -log2(focusScale / viewScale);
    glm::dvec2 center = projection.MetersToLonLat((aMeters + bMeters) * 0.5);
    camera.longitude = center.x;
    camera.latitude = center.y;

    return camera;
}

void Map::flyTo(double _lon, double _lat, float _z, float _duration, float _speed) {

    double lonStart = 0., latStart = 0.;
    getPosition(lonStart, latStart);
    float zStart = getZoom();

    double dLongitude = _lon - lonStart;
    if (dLongitude > 180.0) {
        _lon -= 360.0;
    } else if (dLongitude < -180.0) {
        _lon += 360.0;
    }

    const MapProjection& projection = impl->view.getMapProjection();
    glm::dvec2 a = projection.LonLatToMeters(glm::dvec2(lonStart, latStart));
    glm::dvec2 b = projection.LonLatToMeters(glm::dvec2(_lon, _lat));

    double distance = 0.0;
    auto fn = getFlyToFunction(impl->view,
                               glm::dvec3(a.x, a.y, zStart),
                               glm::dvec3(b.x, b.y, _z),
                               distance);
    auto cb =
        [=](float t) {
            glm::dvec3 pos = fn(t);
            impl->view.setPosition(pos.x, pos.y);
            impl->view.setZoom(pos.z);
            impl->platform->requestRender();
        };

    if (_speed <= 0.f) { _speed = 1.f; }

    float duration = _duration > 0 ? _duration : distance / _speed;

    cancelCameraAnimation();

    impl->eases.push_back(Ease{duration, cb});
}

bool Map::screenPositionToLngLat(double _x, double _y, double* _lng, double* _lat) {

    double intersection = impl->view.screenToGroundPlane(_x, _y);
    glm::dvec3 eye = impl->view.getPosition();
    glm::dvec2 meters(_x + eye.x, _y + eye.y);
    glm::dvec2 lngLat = impl->view.getMapProjection().MetersToLonLat(meters);
    *_lng = LngLat::wrapLongitude(lngLat.x);
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

    // If the pixel scale changes we need to re-build all the tiles.
    // This is expensive, so first check whether the new value is different.
    if (_pixelsPerPoint == view.pixelScale()) {
        // Nothing to do!
        return;
    }
    view.setPixelScale(_pixelsPerPoint);
    scene->setPixelScale(_pixelsPerPoint);

    // Tiles must be rebuilt to apply the new pixel scale to labels.
    tileManager.clearTileSets();

    // Markers must be rebuilt to apply the new pixel scale.
    markerManager.rebuildAll();
}

void Map::setCameraType(int _type) {

    impl->view.setCameraType(static_cast<CameraType>(_type));
    platform->requestRender();

}

int Map::getCameraType() {

    return static_cast<int>(impl->view.cameraType());

}

void Map::addTileSource(std::shared_ptr<TileSource> _source) {
    std::lock_guard<std::mutex> lock(impl->tilesMutex);
    impl->tileManager.addClientTileSource(_source);
}

bool Map::removeTileSource(TileSource& source) {
    std::lock_guard<std::mutex> lock(impl->tilesMutex);
    return impl->tileManager.removeClientTileSource(source);
}

void Map::clearTileSource(TileSource& _source, bool _data, bool _tiles) {
    std::lock_guard<std::mutex> lock(impl->tilesMutex);

    if (_tiles) { impl->tileManager.clearTileSet(_source.id()); }
    if (_data) { _source.clearData(); }

    platform->requestRender();
}

MarkerID Map::markerAdd() {
    return impl->markerManager.add();
}

bool Map::markerRemove(MarkerID _marker) {
    bool success = impl->markerManager.remove(_marker);
    platform->requestRender();
    return success;
}

bool Map::markerSetPoint(MarkerID _marker, LngLat _lngLat) {
    bool success = impl->markerManager.setPoint(_marker, _lngLat);
    platform->requestRender();
    return success;
}

bool Map::markerSetPointEased(MarkerID _marker, LngLat _lngLat, float _duration, EaseType ease) {
    bool success = impl->markerManager.setPointEased(_marker, _lngLat, _duration, ease);
    platform->requestRender();
    return success;
}

bool Map::markerSetPolyline(MarkerID _marker, LngLat* _coordinates, int _count) {
    bool success = impl->markerManager.setPolyline(_marker, _coordinates, _count);
    platform->requestRender();
    return success;
}

bool Map::markerSetPolygon(MarkerID _marker, LngLat* _coordinates, int* _counts, int _rings) {
    bool success = impl->markerManager.setPolygon(_marker, _coordinates, _counts, _rings);
    platform->requestRender();
    return success;
}

bool Map::markerSetStylingFromString(MarkerID _marker, const char* _styling) {
    bool success = impl->markerManager.setStylingFromString(_marker, _styling);
    platform->requestRender();
    return success;
}

bool Map::markerSetStylingFromPath(MarkerID _marker, const char* _path) {
    bool success = impl->markerManager.setStylingFromPath(_marker, _path);
    platform->requestRender();
    return success;
}

bool Map::markerSetBitmap(MarkerID _marker, int _width, int _height, const unsigned int* _data) {
    bool success = impl->markerManager.setBitmap(_marker, _width, _height, _data);
    platform->requestRender();
    return success;
}

bool Map::markerSetVisible(MarkerID _marker, bool _visible) {
    bool success = impl->markerManager.setVisible(_marker, _visible);
    platform->requestRender();
    return success;
}

bool Map::markerSetDrawOrder(MarkerID _marker, int _drawOrder) {
    bool success = impl->markerManager.setDrawOrder(_marker, _drawOrder);
    platform->requestRender();
    return success;
}

void Map::markerRemoveAll() {
    impl->markerManager.removeAll();
    platform->requestRender();
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

    impl->renderState.invalidate();

    impl->tileManager.clearTileSets();

    impl->markerManager.rebuildAll();

    if (impl->selectionBuffer->valid()) {
        impl->selectionBuffer = std::make_unique<FrameBuffer>(impl->selectionBuffer->getWidth(),
                                                              impl->selectionBuffer->getHeight());
    }

    // Set default primitive render color
    Primitives::setColor(impl->renderState, 0xffffff);

    // Load GL extensions and capabilities
    Hardware::loadExtensions();
    Hardware::loadCapabilities();

    // Hardware::printAvailableExtensions();
}

void Map::useCachedGlState(bool _useCache) {
    impl->cacheGlState = _useCache;
}

void Map::runAsyncTask(std::function<void()> _task) {
    if (impl->asyncWorker) {
        impl->asyncWorker->enqueue(std::move(_task));
    }
}

void Map::onMemoryWarning() {

    impl->tileManager.clearTileSets(true);

    if (impl->scene && impl->scene->fontContext()) {
        impl->scene->fontContext()->releaseFonts();
    }
}

void Map::setDefaultBackgroundColor(float r, float g, float b) {
    impl->renderState.defaultOpaqueClearColor(r, g, b);
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
