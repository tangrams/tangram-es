#include "map.h"

#include "debug/textDisplay.h"
#include "debug/frameInfo.h"
#include "gl.h"
#include "gl/glError.h"
#include "gl/framebuffer.h"
#include "gl/hardware.h"
#include "gl/primitives.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "labels/labelManager.h"
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

struct CameraEase {
    struct {
        glm::dvec2 pos;
        float zoom;
        float rotation;
        float tilt;
    } start, end;
};

using CameraAnimator = std::function<uint32_t(float dt)>;

struct ClientTileSource {
    std::shared_ptr<TileSource> tileSource;
    bool added = false;
    bool clear = false;
    bool remove = false;
};


class Map::Impl {
public:
    explicit Impl(Platform& _platform) :
        platform(_platform),
        inputHandler(view),
        scene(std::make_shared<Scene>(_platform)) {}

    void setPixelScale(float _pixelsPerPoint);
    SceneID loadScene(SceneOptions&& _sceneOptions);
    SceneID loadSceneAsync(SceneOptions&& _sceneOptions);
    void syncClientTileSources(bool _firstUpdate);
    bool updateCameraEase(float _dt);

    std::mutex sceneMutex;

    Platform& platform;
    RenderState renderState;
    JobQueue jobQueue;
    // Current interactive view
    View view;
    std::unique_ptr<AsyncWorker> asyncWorker = std::make_unique<AsyncWorker>();
    InputHandler inputHandler;

    std::unique_ptr<Ease> ease;

    std::shared_ptr<Scene> scene;
    std::condition_variable blockUntilSceneReady;

    // NB: Destruction of (managed and loading) tiles must happen
    // before implicit destruction of 'scene' above!
    // In particular any references of Labels and Markers to FontContext
    std::unique_ptr<FrameBuffer> selectionBuffer = std::make_unique<FrameBuffer>(0, 0);

    bool cacheGlState = false;
    float pickRadius = .5f;
    bool isAnimating = false;

    std::vector<SelectionQuery> selectionQueries;

    SceneReadyCallback onSceneReady = nullptr;
    CameraAnimationCallback cameraAnimationListener = nullptr;

    std::mutex tileSourceMutex;

    std::map<int32_t, ClientTileSource> clientTileSources;

    // TODO MapOption
    Color background{0xffffffff};
};


static std::bitset<9> g_flags = 0;

Map::Map(std::unique_ptr<Platform> _platform) : platform(std::move(_platform)) {
    LOGTOInit();
    impl.reset(new Impl(*platform));
}

Map::~Map() {
    // Let the platform stop all outstanding tasks:
    // Send cancel to UrlRequests so any thread blocking on a response can join,
    // and discard incoming UrlRequest directly.
    //
    // In any case after shutdown Platform may not call back into Map!
    platform->shutdown();

    // The unique_ptr to Impl will be automatically destroyed when Map is destroyed.
    impl->asyncWorker.reset();

    impl->scene.reset();
    impl->blockUntilSceneReady.notify_all();

    // Make sure other threads are stopped before calling stop()!
    // All jobs will be executed immediately on add() afterwards.
    impl->jobQueue.stop();

    TextDisplay::Instance().deinit();
    Primitives::deinit();
}


SceneID Map::loadScene(SceneOptions&& _sceneOptions, bool _async) {
    if (_async) {
        return impl->loadSceneAsync(std::move(_sceneOptions));
    } else {
        return impl->loadScene(std::move(_sceneOptions));
    }
}

SceneID Map::Impl::loadScene(SceneOptions&& _sceneOptions) {

    /// Avoid to keep loading old scene and tiles
    scene->dispose();

    scene = std::make_shared<Scene>(platform, std::move(_sceneOptions));

    scene->load();

    if (onSceneReady) {
        onSceneReady(scene->id, scene->errors());
    }

    return scene->id;
}

SceneID Map::Impl::loadSceneAsync(SceneOptions&& _sceneOptions) {

    /// Avoid to keep loading old scene and tiles
    auto oldScene = std::move(scene);
    oldScene->cancelTasks();

    /// Add callback for tile prefetching
    _sceneOptions.asyncCallback = [&](Scene* _scene) {
        jobQueue.add([&, _scene]() {
            if (_scene == scene.get()) {
                scene->prefetchTiles(view);
                background = scene->backgroundColor(view.getIntegerZoom());
            }});
        platform.requestRender();
    };

    scene = std::make_shared<Scene>(platform, std::move(_sceneOptions));

    asyncWorker->enqueue([this, newScene = scene]() {
        LOG("ASYNC LOAD >>>");

        newScene->load();

        platform.requestRender();

        if (onSceneReady) {
            onSceneReady(newScene->id, newScene->errors());
        }

        /// Check if another Scene is already queued
        if (newScene != scene) {
            newScene->dispose();
        }

        LOG("ASYNC LOAD <<<");
    });

    /// Disposing TileWorker is blocking: Do this async just in case
    asyncWorker->enqueue([s = std::move(oldScene)]() mutable {
        LOG("ASYNC DISPOSE OLD SCENE");
        s->dispose();
    });

    return scene->id;
}

void Map::setSceneReadyListener(SceneReadyCallback _onSceneReady) {
    impl->onSceneReady = _onSceneReady;
}

void Map::setCameraAnimationListener(CameraAnimationCallback _cb) {
    impl->cameraAnimationListener = _cb;
}

Platform& Map::getPlatform() {
    return *platform;
}

void Map::resize(int _newWidth, int _newHeight) {

    LOGS("resize: %d x %d", _newWidth, _newHeight);
    LOG("resize: %d x %d", _newWidth, _newHeight);

    impl->view.setSize(_newWidth, _newHeight);

    impl->selectionBuffer = std::make_unique<FrameBuffer>(_newWidth/2, _newHeight/2);
}

MapState Map::update(float _dt) {

    FrameInfo::beginUpdate();

    impl->jobQueue.runJobs();

    bool isEasing = impl->updateCameraEase(_dt);
    bool isFlinging = impl->inputHandler.update(_dt);

    uint32_t state = 0;
    if (isEasing || isFlinging) {
        state |= MapState::view_changing;
        state |= MapState::is_animating;
    }

    impl->view.update();

    auto& scene = *impl->scene;
    bool wasReady = scene.isReady();

    if (!scene.completeScene(impl->view)) {
        state |= MapState::scene_pending;
    } else {
        // Sync ClientTileSource changes with TileManager
        bool firstUpdate = !wasReady;
        impl->syncClientTileSources(firstUpdate);

        auto sceneState = scene.update(impl->view, _dt);

        // Sets tiles_loading, animate_labels, animate_markers
        state |= sceneState.flags;
    }

    FrameInfo::endUpdate();

    return { state };
}

void Map::render() {

    auto& scene = *impl->scene;
    auto& view = impl->view;
    auto& renderState = impl->renderState;

    glm::vec2 viewport(view.getWidth(), view.getHeight());

    // Delete batch of gl resources
    renderState.flushResourceDeletion();

    // Invalidate render states for new frame
    if (!impl->cacheGlState) {
        renderState.invalidateStates();
    }

    // Cache default framebuffer handle used for rendering
    renderState.cacheDefaultFramebuffer();

    // Do not render while scene is loading
    if (!scene.isReady()) {
        FrameBuffer::apply(renderState, renderState.defaultFrameBuffer(),
                           viewport, impl->background.toColorF());
        return;
    }

    Primitives::setResolution(renderState, view.getWidth(), view.getHeight());
    FrameInfo::beginFrame();

    scene.renderBeginFrame(renderState);

    // Render feature selection pass to offscreen framebuffer
    bool drawSelectionDebug = getDebugFlag(DebugFlags::selection_buffer);
    bool drawSelectionBuffer = !impl->selectionQueries.empty();

    if (drawSelectionBuffer || drawSelectionDebug) {
        impl->selectionBuffer->applyAsRenderTarget(impl->renderState);

        scene.renderSelection(renderState, view,
                              *impl->selectionBuffer,
                              impl->selectionQueries);

        impl->selectionQueries.clear();
    }

    // Get background color for frame based on zoom level, if there are stops
    impl->background = scene.backgroundColor(view.getIntegerZoom());

    // Setup default framebuffer for a new frame
    FrameBuffer::apply(renderState, renderState.defaultFrameBuffer(),
                       viewport, impl->background.toColorF());

    if (drawSelectionDebug) {
        impl->selectionBuffer->drawDebug(renderState, viewport);
        FrameInfo::draw(renderState, view, *scene.tileManager());
        return;
    }

    // Render scene
    bool drawnAnimatedStyle = scene.render(renderState, view);

    if (scene.animated() != Scene::animate::no &&
        drawnAnimatedStyle != platform->isContinuousRendering()) {
        platform->setContinuousRendering(drawnAnimatedStyle);
    }

    FrameInfo::draw(renderState, view, *scene.tileManager());
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
    GL::readPixels(0, 0, impl->view.getWidth(), impl->view.getHeight(), GL_RGBA,
                   GL_UNSIGNED_BYTE, (GLvoid*)_data);
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

    impl->ease.reset();

    if (impl->cameraAnimationListener) {
        impl->cameraAnimationListener(false);
    }
}

void Map::setCameraPosition(const CameraPosition& _camera) {
    cancelCameraAnimation();

    impl->view.setCenterCoordinates(LngLat(_camera.longitude, _camera.latitude));
    impl->view.setZoom(_camera.zoom);
    impl->view.setRoll(_camera.rotation);
    impl->view.setPitch(_camera.tilt);

    impl->platform.requestRender();
}

void Map::setCameraPositionEased(const CameraPosition& _camera, float _duration, EaseType _e) {
    cancelCameraAnimation();

    CameraEase e;

    double lonStart, latStart;
    getPosition(lonStart, latStart);

    double lonEnd = _camera.longitude;
    double latEnd = _camera.latitude;

    double dLongitude = lonEnd - lonStart;
    if (dLongitude > 180.0) {
        lonEnd -= 360.0;
    } else if (dLongitude < -180.0) {
        lonEnd += 360.0;
    }

    e.start.pos = MapProjection::lngLatToProjectedMeters({lonStart, latStart});
    e.end.pos = MapProjection::lngLatToProjectedMeters({lonEnd, latEnd});

    e.start.zoom = getZoom();
    e.end.zoom = glm::clamp(_camera.zoom, getMinZoom(), getMaxZoom());

    float radiansStart = getRotation();

    // Ease over the smallest angular distance needed
    float radiansDelta = glm::mod(_camera.rotation - radiansStart, (float)TWO_PI);
    if (radiansDelta > PI) { radiansDelta -= TWO_PI; }

    e.start.rotation = radiansStart;
    e.end.rotation = radiansStart + radiansDelta;

    e.start.tilt = getTilt();
    e.end.tilt = _camera.tilt;

    impl->ease = std::make_unique<Ease>(_duration,
        [=](float t) {
            impl->view.setPosition(ease(e.start.pos.x, e.end.pos.x, t, _e),
                                   ease(e.start.pos.y, e.end.pos.y, t, _e));
            impl->view.setZoom(ease(e.start.zoom, e.end.zoom, t, _e));

            impl->view.setRoll(ease(e.start.rotation, e.end.rotation, t, _e));

            impl->view.setPitch(ease(e.start.tilt, e.end.tilt, t, _e));
        });

    platform->requestRender();
}

bool Map::Impl::updateCameraEase(float _dt) {
    if (!ease) { return false; }

    ease->update(_dt);

    if (ease->finished()) {
        if (cameraAnimationListener) {
            cameraAnimationListener(true);
        }
        ease.reset();
        return false;
    }
    return true;
}

void Map::updateCameraPosition(const CameraUpdate& _update, float _duration, EaseType _e) {

    CameraPosition camera{};
    if ((_update.set & CameraUpdate::set_camera) != 0) {
        camera = getCameraPosition();
    }
    if ((_update.set & CameraUpdate::set_bounds) != 0) {
        camera = getEnclosingCameraPosition(_update.bounds[0], _update.bounds[1], _update.padding);
    }
    if ((_update.set & CameraUpdate::set_lnglat) != 0) {
        camera.longitude = _update.lngLat.longitude;
        camera.latitude = _update.lngLat.latitude;
    }
    if ((_update.set & CameraUpdate::set_zoom) != 0) {
        camera.zoom = _update.zoom;
    }
    if ((_update.set & CameraUpdate::set_rotation) != 0) {
        camera.rotation = _update.rotation;
    }
    if ((_update.set & CameraUpdate::set_tilt) != 0) {
        camera.tilt = _update.tilt;
    }
    if ((_update.set & CameraUpdate::set_zoom_by) != 0) {
        camera.zoom += _update.zoomBy;
    }
    if ((_update.set & CameraUpdate::set_rotation_by) != 0) {
        camera.rotation += _update.rotationBy;
    }
    if ((_update.set & CameraUpdate::set_tilt_by) != 0) {
        camera.tilt += _update.tiltBy;
    }

    if (_duration == 0.f) {
        setCameraPosition(camera);
    } else {
        setCameraPositionEased(camera, _duration, _e);
    }
}

void Map::setPosition(double _lon, double _lat) {
    cancelCameraAnimation();

    glm::dvec2 meters = MapProjection::lngLatToProjectedMeters({_lon, _lat});
    impl->view.setPosition(meters.x, meters.y);
    impl->inputHandler.cancelFling();
    impl->platform.requestRender();
}

void Map::getPosition(double& _lon, double& _lat) {
    LngLat degrees = impl->view.getCenterCoordinates();
    _lon = degrees.longitude;
    _lat = degrees.latitude;
}

void Map::setZoom(float _z) {
    cancelCameraAnimation();

    impl->view.setZoom(_z);
    impl->inputHandler.cancelFling();
    impl->platform.requestRender();
}

float Map::getZoom() {
    return impl->view.getZoom();
}

void Map::setMinZoom(float _minZoom) {
    impl->view.setMinZoom(_minZoom);
}

float Map::getMinZoom() {
    return impl->view.getMinZoom();
}

void Map::setMaxZoom(float _maxZoom) {
    impl->view.setMaxZoom(_maxZoom);
}

float Map::getMaxZoom() {
    return impl->view.getMaxZoom();
}

void Map::setRotation(float _radians) {
    cancelCameraAnimation();

    impl->view.setRoll(_radians);
    impl->platform.requestRender();
}

float Map::getRotation() {
    return impl->view.getRoll();
}

void Map::setTilt(float _radians) {
    cancelCameraAnimation();

    impl->view.setPitch(_radians);
    impl->platform.requestRender();
}

float Map::getTilt() {
    return impl->view.getPitch();
}

CameraPosition Map::getEnclosingCameraPosition(LngLat _a, LngLat _b, EdgePadding _pad) {
    const View& view = impl->view;

    // Convert the bounding coordinates into Mercator meters.
    ProjectedMeters aMeters = MapProjection::lngLatToProjectedMeters(_a);
    ProjectedMeters bMeters = MapProjection::lngLatToProjectedMeters(_b);
    ProjectedMeters dMeters = glm::abs(aMeters - bMeters);

    // Calculate the inner size of the view that the bounds must fit within.
    glm::dvec2 innerSize(view.getWidth() / view.pixelScale(), view.getHeight() / view.pixelScale());
    innerSize -= glm::dvec2((_pad.left + _pad.right), (_pad.top + _pad.bottom));

    // Calculate the map scale that fits the bounds into the inner size in each dimension.
    glm::dvec2 metersPerPixel = dMeters / innerSize;

    // Take the value from the larger dimension to calculate the final zoom.
    double maxMetersPerPixel = std::max(metersPerPixel.x, metersPerPixel.y);
    double zoom = MapProjection::zoomAtMetersPerPixel(maxMetersPerPixel);
    double finalZoom = glm::clamp(zoom, (double)getMinZoom(), (double)getMaxZoom());
    double finalMetersPerPixel = MapProjection::metersPerPixelAtZoom(finalZoom);

    // Adjust the center of the final visible region using the padding converted to Mercator meters.
    glm::dvec2 paddingMeters = glm::dvec2(_pad.right - _pad.left, _pad.top - _pad.bottom) * finalMetersPerPixel;
    glm::dvec2 centerMeters = 0.5 * (aMeters + bMeters + paddingMeters);

    LngLat centerLngLat = MapProjection::projectedMetersToLngLat(centerMeters);

    CameraPosition camera;
    camera.zoom = static_cast<float>(finalZoom);
    camera.longitude = centerLngLat.longitude;
    camera.latitude = centerLngLat.latitude;
    return camera;
}

void Map::flyTo(const CameraPosition& _camera, float _duration, float _speed) {

    double lngStart = 0., latStart = 0., lngEnd = _camera.longitude, latEnd = _camera.latitude;
    getPosition(lngStart, latStart);
    float zStart = getZoom();
    float rStart = getRotation();
    float tStart = getTilt();

    // Ease over the smallest angular distance needed
    float radiansDelta = glm::mod(_camera.rotation - rStart, (float)TWO_PI);
    if (radiansDelta > PI) { radiansDelta -= TWO_PI; }
    float rEnd = rStart + radiansDelta;

    double dLongitude = lngEnd - lngStart;
    if (dLongitude > 180.0) {
        lngEnd -= 360.0;
    } else if (dLongitude < -180.0) {
        lngEnd += 360.0;
    }

    ProjectedMeters a = MapProjection::lngLatToProjectedMeters(LngLat(lngStart, latStart));
    ProjectedMeters b = MapProjection::lngLatToProjectedMeters(LngLat(lngEnd, latEnd));

    double distance = 0.0;
    auto fn = getFlyToFunction(impl->view,
                               glm::dvec3(a.x, a.y, zStart),
                               glm::dvec3(b.x, b.y, _camera.zoom),
                               distance);

    EaseType e = EaseType::cubic;
    auto cb =
        [=](float t) {
            glm::dvec3 pos = fn(t);
            impl->view.setPosition(pos.x, pos.y);
            impl->view.setZoom(pos.z);
            impl->view.setRoll(ease(rStart, rEnd, t, e));
            impl->view.setPitch(ease(tStart, _camera.tilt, t, e));
            impl->platform.requestRender();
        };

    if (_speed <= 0.f) { _speed = 1.f; }

    float duration = _duration > 0 ? _duration : distance / _speed;

    cancelCameraAnimation();

    impl->ease = std::make_unique<Ease>(duration, cb);

    platform->requestRender();
}

bool Map::screenPositionToLngLat(double _x, double _y, double* _lng, double* _lat) {

    bool intersection = false;
    LngLat lngLat = impl->view.screenPositionToLngLat(_x, _y, intersection);
    *_lng = lngLat.longitude;
    *_lat = lngLat.latitude;

    return intersection;
}

bool Map::lngLatToScreenPosition(double _lng, double _lat, double* _x, double* _y) {
    bool clipped = false;

    glm::vec2 screenCoords = impl->view.lngLatToScreenPosition(_lng, _lat, clipped);

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
}

void Map::setCameraType(int _type) {
    impl->view.setCameraType(static_cast<CameraType>(_type));
    platform->requestRender();
}

int Map::getCameraType() {
    return static_cast<int>(impl->view.cameraType());
}

void Map::addTileSource(std::shared_ptr<TileSource> _source) {
    std::lock_guard<std::mutex> lock(impl->tileSourceMutex);

    auto& tileSources = impl->clientTileSources;
    auto& entry = tileSources[_source->id()];

    entry.tileSource = _source;
    entry.added = true;
}

bool Map::removeTileSource(TileSource& _source) {
    std::lock_guard<std::mutex> lock(impl->tileSourceMutex);

    auto& tileSources = impl->clientTileSources;
    auto it = tileSources.find(_source.id());
    if (it != tileSources.end()) {
        it->second.remove = true;
        return true;
    }
    return false;
}

bool Map::clearTileSource(TileSource& _source, bool _data, bool _tiles) {
    std::lock_guard<std::mutex> lock(impl->tileSourceMutex);

    if (_data) { _source.clearData(); }
    if (!_tiles) { return true; }

    auto& tileSources = impl->clientTileSources;
    auto it = tileSources.find(_source.id());
    if (it != tileSources.end()) {
        it->second.clear = true;
        return true;
    }
    return false;
}

void Map::Impl::syncClientTileSources(bool _firstUpdate) {
    std::lock_guard<std::mutex> lock(tileSourceMutex);

    auto& tileManager = *scene->tileManager();
    for (auto it = clientTileSources.begin();
         it != clientTileSources.end(); ) {
        auto& ts = it->second;
        if (ts.remove) {
            tileManager.removeClientTileSource(it->first);
            it = clientTileSources.erase(it);
            continue;
        }
        if (ts.added || _firstUpdate) {
            ts.added = false;
            tileManager.addClientTileSource(ts.tileSource);
        }
        if (ts.clear) {
            ts.clear = false;
            tileManager.clearTileSet(it->first);
        }
        ++it;
    }
}

MarkerID Map::markerAdd() {
    return impl->scene->markerManager()->add();
}

bool Map::markerRemove(MarkerID _marker) {
    bool success = impl->scene->markerManager()->remove(_marker);
    platform->requestRender();
    return success;
}

bool Map::markerSetPoint(MarkerID _marker, LngLat _lngLat) {
    bool success = impl->scene->markerManager()->setPoint(_marker, _lngLat);
    platform->requestRender();
    return success;
}

bool Map::markerSetPointEased(MarkerID _marker, LngLat _lngLat, float _duration, EaseType ease) {
    bool success = impl->scene->markerManager()->setPointEased(_marker, _lngLat, _duration, ease);
    platform->requestRender();
    return success;
}

bool Map::markerSetPolyline(MarkerID _marker, LngLat* _coordinates, int _count) {
    bool success = impl->scene->markerManager()->setPolyline(_marker, _coordinates, _count);
    platform->requestRender();
    return success;
}

bool Map::markerSetPolygon(MarkerID _marker, LngLat* _coordinates, int* _counts, int _rings) {
    bool success = impl->scene->markerManager()->setPolygon(_marker, _coordinates, _counts, _rings);
    platform->requestRender();
    return success;
}

bool Map::markerSetStylingFromString(MarkerID _marker, const char* _styling) {
    bool success = impl->scene->markerManager()->setStylingFromString(_marker, _styling);
    platform->requestRender();
    return success;
}

bool Map::markerSetStylingFromPath(MarkerID _marker, const char* _path) {
    bool success = impl->scene->markerManager()->setStylingFromPath(_marker, _path);
    platform->requestRender();
    return success;
}

bool Map::markerSetBitmap(MarkerID _marker, int _width, int _height, const unsigned int* _data, float _density) {
    bool success = impl->scene->markerManager()->setBitmap(_marker, _width, _height, _density, _data);
    platform->requestRender();
    return success;
}

bool Map::markerSetVisible(MarkerID _marker, bool _visible) {
    bool success = impl->scene->markerManager()->setVisible(_marker, _visible);
    platform->requestRender();
    return success;
}

bool Map::markerSetDrawOrder(MarkerID _marker, int _drawOrder) {
    bool success = impl->scene->markerManager()->setDrawOrder(_marker, _drawOrder);
    platform->requestRender();
    return success;
}

void Map::markerRemoveAll() {
    impl->scene->markerManager()->removeAll();
    platform->requestRender();
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

void Map::handleTapGesture(float _posX, float _posY) {
    cancelCameraAnimation();
    impl->inputHandler.handleTapGesture(_posX, _posY);
    impl->platform.requestRender();
}

void Map::handleDoubleTapGesture(float _posX, float _posY) {
    cancelCameraAnimation();
    impl->inputHandler.handleDoubleTapGesture(_posX, _posY);
    impl->platform.requestRender();
}

void Map::handlePanGesture(float _startX, float _startY, float _endX, float _endY) {
    cancelCameraAnimation();
    impl->inputHandler.handlePanGesture(_startX, _startY, _endX, _endY);
    impl->platform.requestRender();
}

void Map::handleFlingGesture(float _posX, float _posY, float _velocityX, float _velocityY) {
    cancelCameraAnimation();
    impl->inputHandler.handleFlingGesture(_posX, _posY, _velocityX, _velocityY);
    impl->platform.requestRender();
}

void Map::handlePinchGesture(float _posX, float _posY, float _scale, float _velocity) {
    cancelCameraAnimation();
    impl->inputHandler.handlePinchGesture(_posX, _posY, _scale, _velocity);
    impl->platform.requestRender();
}

void Map::handleRotateGesture(float _posX, float _posY, float _radians) {
    cancelCameraAnimation();
    impl->inputHandler.handleRotateGesture(_posX, _posY, _radians);
    impl->platform.requestRender();
}

void Map::handleShoveGesture(float _distance) {
    cancelCameraAnimation();
    impl->inputHandler.handleShoveGesture(_distance);
    impl->platform.requestRender();
}

void Map::setupGL() {

    LOG("setup GL");

    impl->renderState.invalidate();

    //impl->scene->tileManager()->clearTileSets();
    impl->scene->markerManager()->rebuildAll();

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

    impl->scene->tileManager()->clearTileSets(true);

    if (impl->scene && impl->scene->fontContext()) {
        impl->scene->fontContext()->releaseFonts();
    }
}

void Map::setDefaultBackgroundColor(float r, float g, float b) {
    impl->renderState.defaultOpaqueClearColor(r, g, b);
}

void setDebugFlag(DebugFlags _flag, bool _on) {

    g_flags.set(_flag, _on);
    // m_view.setZoom(m_view.getZoom()); // Force the view to refresh

}

bool getDebugFlag(DebugFlags _flag) {

    return g_flags.test(_flag);

}

void toggleDebugFlag(DebugFlags _flag) {

    g_flags.flip(_flag);
    // m_view.setZoom(m_view.getZoom()); // Force the view to refresh

    // Rebuild tiles for debug modes that needs it
    // if (_flag == DebugFlags::proxy_colors
    //  || _flag == DebugFlags::draw_all_labels
    //  || _flag == DebugFlags::tile_bounds
    //  || _flag == DebugFlags::tile_infos) {
    //     if (m_tileManager) {
    //         m_tileManager->clearTileSets();
    //     }
    // }
}

}
