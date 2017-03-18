#include "tangram.h"

#include "data/clientGeoJsonSource.h"
#include "debug/textDisplay.h"
#include "debug/frameInfo.h"
#include "gl.h"
#include "gl/error.h"
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
#include "view/view.h"

#include <bitset>
#include <cmath>

namespace Tangram {

const static size_t MAX_WORKERS = 2;

enum class EaseField { position, zoom, rotation, tilt };

class Map::Impl {

public:
    Impl(std::shared_ptr<Platform> _platform) :
        platform(_platform),
        inputHandler(_platform, view),
        scene(std::make_shared<Scene>(_platform)),
        tileWorker(_platform, MAX_WORKERS),
        tileManager(_platform, tileWorker) {}

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

    RenderState renderState;
    JobQueue jobQueue;
    View view;
    Labels labels;
    std::unique_ptr<AsyncWorker> asyncWorker = std::make_unique<AsyncWorker>();
    std::shared_ptr<Platform> platform;
    InputHandler inputHandler;

    std::vector<SceneUpdate> sceneUpdates;
    std::array<Ease, 4> eases;

    std::shared_ptr<Scene> scene;
    std::shared_ptr<Scene> nextScene = nullptr;

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
};

void Map::Impl::setEase(EaseField _f, Ease _e) {
    eases[static_cast<size_t>(_f)] = _e;
    platform->requestRender();
}

void Map::Impl::clearEase(EaseField _f) {
    static Ease none = {};
    eases[static_cast<size_t>(_f)] = none;
}

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
void Map::loadScene(const char* _scenePath, bool _useScenePosition,
                    const std::vector<SceneUpdate>& _sceneUpdates) {

    LOG("Loading scene file: %s", _scenePath);

    {
        std::lock_guard<std::mutex> lock(impl->sceneMutex);
        impl->sceneUpdates.clear();
        impl->nextScene.reset();
    }

    auto scene = std::make_shared<Scene>(platform, _scenePath);
    scene->useScenePosition = _useScenePosition;

    if (SceneLoader::loadScene(platform, scene, _sceneUpdates)) {
        impl->setScene(scene);
    }
}

void Map::loadSceneAsync(const char* _scenePath, bool _useScenePosition,
                         MapReady _platformCallback, void *_cbData,
                         const std::vector<SceneUpdate>& _sceneUpdates) {

    LOG("Loading scene file (async): %s", _scenePath);

    std::shared_ptr<Scene> nextScene;
    {
        std::lock_guard<std::mutex> lock(impl->sceneMutex);
        impl->sceneUpdates.clear();
        impl->nextScene = std::make_shared<Scene>(platform, _scenePath);
        impl->nextScene->useScenePosition = _useScenePosition;
        nextScene = impl->nextScene;
    }

    runAsyncTask([nextScene, _platformCallback, _cbData, _sceneUpdates, this](){

            bool ok = SceneLoader::loadScene(platform, nextScene, _sceneUpdates);

            impl->jobQueue.add([nextScene, ok, _platformCallback, _cbData, this]() {
                    {
                        std::lock_guard<std::mutex> lock(impl->sceneMutex);
                        if (nextScene == impl->nextScene) {
                            impl->nextScene.reset();
                        } else {
                            // loadScene[Async] was called in the meantime.
                            return;
                        }
                    }

                    if (ok) {
                        auto s = nextScene;
                        impl->setScene(s);
                        applySceneUpdates();
                        if (_platformCallback) { _platformCallback(_cbData); }
                    }
                });
            platform->requestRender();
        });
}

void Map::queueSceneUpdate(const char* _path, const char* _value) {
    std::lock_guard<std::mutex> lock(impl->sceneMutex);
    impl->sceneUpdates.push_back({_path, _value});
}

void Map::queueSceneUpdate(const std::vector<SceneUpdate>& sceneUpdates) {
    std::lock_guard<std::mutex> lock(impl->sceneMutex);
    impl->sceneUpdates.insert(impl->sceneUpdates.end(), sceneUpdates.begin(), sceneUpdates.end());
}

std::shared_ptr<Platform>& Map::getPlatform() {
    return platform;
}

void Map::applySceneUpdates() {

    std::shared_ptr<Scene> nextScene;
    std::vector<SceneUpdate> updates;
    {
        std::lock_guard<std::mutex> lock(impl->sceneMutex);
        if (impl->sceneUpdates.empty()) { return; }

        if (impl->nextScene) {
            // Changes are automatically applied once the scene is loaded
            return;
        }
        LOG("Applying %d scene updates", impl->sceneUpdates.size());

        impl->nextScene = std::make_shared<Scene>(*impl->scene);
        impl->nextScene->useScenePosition = false;

        updates = impl->sceneUpdates;
        impl->sceneUpdates.clear();

        nextScene = impl->nextScene;
    }

    runAsyncTask([nextScene, updates = std::move(updates), this](){

            SceneLoader::applyUpdates(*nextScene, updates);

            bool ok = SceneLoader::applyConfig(platform, nextScene);

            impl->jobQueue.add([nextScene, ok, this]() {
                    {
                        std::lock_guard<std::mutex> lock(impl->sceneMutex);
                        if (nextScene == impl->nextScene) {
                            impl->nextScene.reset();
                        } else {
                            // loadScene[Async] was called in the meantime.
                            return;
                        }
                    }

                    if (ok) {
                        auto s = nextScene;
                        impl->setScene(s);
                        applySceneUpdates();
                    }
                });
            platform->requestRender();
        });
}

void Map::setMBTiles(const char* _dataSourceName, const char* _mbtilesFilePath) {
    std::string scenePath = std::string("sources.") + _dataSourceName + ".mbtiles";
    queueSceneUpdate(scenePath.c_str(), _mbtilesFilePath);
    applySceneUpdates();
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

    // Wait until font resources are fully loaded
    if (impl->scene->pendingFonts > 0) {
        platform->requestRender();
        return false;
    }

    FrameInfo::beginUpdate();

    impl->jobQueue.runJobs();

    impl->scene->updateTime(_dt);

    bool viewComplete = true;
    bool markersNeedUpdate = false;

    for (auto& ease : impl->eases) {
        if (!ease.finished()) {
            ease.update(_dt);
            viewComplete = false;
        }
    }

    impl->inputHandler.update(_dt);

    impl->view.update();

    impl->markerManager.update(static_cast<int>(impl->view.getZoom()));

    for (const auto& style : impl->scene->styles()) {
        style->onBeginUpdate();
    }

    {
        std::lock_guard<std::mutex> lock(impl->tilesMutex);

        impl->tileManager.updateTileSets(impl->view.state(), impl->view.getVisibleTiles());

        auto& tiles = impl->tileManager.getVisibleTiles();
        auto& markers = impl->markerManager.markers();

        for (const auto& marker : markers) {
            marker->update(_dt, impl->view);
            markersNeedUpdate |= marker->isEasing();
        }

        if (impl->view.changedOnLastUpdate() ||
            impl->tileManager.hasTileSetChanged()) {

            for (const auto& tile : tiles) {
                tile->update(_dt, impl->view);
            }
            impl->labels.updateLabelSet(impl->view.state(), _dt, impl->scene->styles(), tiles, markers,
                                        *impl->tileManager.getTileCache());
        } else {
            impl->labels.updateLabels(impl->view.state(), _dt, impl->scene->styles(), tiles, markers);
        }
    }

    FrameInfo::endUpdate();

    bool viewChanged = impl->view.changedOnLastUpdate();
    bool tilesChanged = impl->tileManager.hasTileSetChanged();
    bool tilesLoading = impl->tileManager.hasLoadingTiles();
    bool labelsNeedUpdate = impl->labels.needUpdate();
    bool resourceLoading = (impl->scene->pendingTextures > 0);
    bool nextScene = bool(impl->nextScene);

    if (viewChanged || tilesChanged || tilesLoading || labelsNeedUpdate || resourceLoading || nextScene) {
        viewComplete = false;
    }

    // Request render if labels are in fading states or markers are easing.
    if (labelsNeedUpdate || markersNeedUpdate) { platform->requestRender(); }

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
        impl->renderState.invalidate();
    }

    // Run render-thread tasks
    impl->renderState.jobQueue.runJobs();

    // Render feature selection pass to offscreen framebuffer
    if (impl->selectionQueries.size() > 0 || drawSelectionBuffer) {
        impl->selectionBuffer->applyAsRenderTarget(impl->renderState);

        std::lock_guard<std::mutex> lock(impl->tilesMutex);

        for (const auto& style : impl->scene->styles()) {
            style->onBeginDrawSelectionFrame(impl->renderState, impl->view, *(impl->scene));

            for (const auto& tile : impl->tileManager.getVisibleTiles()) {
                style->drawSelectionFrame(impl->renderState, *tile);
            }

            for (const auto& marker : impl->markerManager.markers()) {
                style->drawSelectionFrame(impl->renderState, *marker);
            }
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
                       viewport, impl->scene->background().asIVec4());

    if (drawSelectionBuffer) {
        impl->selectionBuffer->drawDebug(impl->renderState, viewport);
        FrameInfo::draw(impl->renderState, impl->view, impl->tileManager);
        return;
    }

    for (const auto& style : impl->scene->styles()) {
        style->onBeginFrame(impl->renderState);
    }

    {
        std::lock_guard<std::mutex> lock(impl->tilesMutex);

        // Loop over all styles
        for (const auto& style : impl->scene->styles()) {

            style->onBeginDrawFrame(impl->renderState, impl->view, *(impl->scene));

            // Loop over all tiles in m_tileSet
            for (const auto& tile : impl->tileManager.getVisibleTiles()) {
                style->draw(impl->renderState, *tile);
            }

            for (const auto& marker : impl->markerManager.markers()) {
                style->draw(impl->renderState, *marker);
            }

            style->onEndDrawFrame();
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

void Map::Impl::setPositionNow(double _lon, double _lat) {

    glm::dvec2 meters = view.getMapProjection().LonLatToMeters({ _lon, _lat});
    view.setPosition(meters.x, meters.y);
    inputHandler.cancelFling();
    platform->requestRender();

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
    platform->requestRender();

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
    platform->requestRender();

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
    platform->requestRender();

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
