#include "tangram.h"

#include "platform.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "scene/skybox.h"
#include "style/material.h"
#include "style/style.h"
#include "labels/labels.h"
#include "tile/tileManager.h"
#include "tile/tile.h"
#include "gl/error.h"
#include "gl/shaderProgram.h"
#include "gl/renderState.h"
#include "gl/primitives.h"
#include "gl/hardware.h"
#include "util/inputHandler.h"
#include "tile/tileCache.h"
#include "view/view.h"
#include "data/clientGeoJsonSource.h"
#include "text/fontContext.h"
#include "gl.h"
#include "gl/hardware.h"
#include "util/ease.h"
#include "debug/textDisplay.h"
#include <memory>
#include <array>
#include <cmath>
#include <bitset>
#include <mutex>
#include <queue>


namespace Tangram {

const static size_t MAX_WORKERS = 2;

std::mutex m_tilesMutex;
std::mutex m_tasksMutex;
std::queue<std::function<void()>> m_tasks;
std::unique_ptr<TileManager> m_tileManager;
std::unique_ptr<TileWorker> m_tileWorker;
std::shared_ptr<Scene> m_scene;
std::shared_ptr<View> m_view;
std::unique_ptr<Labels> m_labels;
std::unique_ptr<Skybox> m_skybox;
std::unique_ptr<InputHandler> m_inputHandler;

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
int log_level = 2;

void initialize(const char* _scenePath) {

    if (m_tileManager) {
        LOG("Notice: Already initialized");
        return;
    }

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

    // label setup
    m_labels = std::make_unique<Labels>();

    loadScene(_scenePath, true);

    glm::dvec2 projPos = m_view->getMapProjection().LonLatToMeters(m_scene->startPosition);
    m_view->setPosition(projPos.x, projPos.y);
    m_view->setZoom(m_scene->startZoom);

    LOG("finish initialize");

}

void loadScene(const char* _scenePath, bool _setPositionFromScene) {
    LOG("Loading scene file: %s", _scenePath);

    auto sceneString = stringFromFile(setResourceRoot(_scenePath).c_str(), PathType::resource);

    bool setPositionFromCurrentView = bool(m_scene);

    auto scene = std::make_shared<Scene>();
    if (m_view) {
        scene->view() = std::make_shared<View>(*m_view);
    }
    if (SceneLoader::loadScene(sceneString, *scene)) {
        m_scene = scene;
        m_scene->fontContext()->addFont("firasans", "medium", "");
        if (setPositionFromCurrentView && !_setPositionFromScene) {
            m_scene->view()->setPosition(m_view->getPosition());
            m_scene->view()->setZoom(m_view->getZoom());
        }
        m_view = m_scene->view();
        m_inputHandler->setView(m_view);
        m_tileManager->setDataSources(scene->dataSources());
        setPixelScale(m_view->pixelScale());

        m_tileWorker->setScene(scene);
    }
}

void resize(int _newWidth, int _newHeight) {

    LOGS("resize: %d x %d", _newWidth, _newHeight);
    LOG("resize: %d x %d", _newWidth, _newHeight);

    glViewport(0, 0, _newWidth, _newHeight);

    if (m_view) {
        m_view->setSize(_newWidth, _newHeight);
    }

    Primitives::setResolution(_newWidth, _newHeight);

    while (Error::hadGlError("Tangram::resize()")) {}

}

void update(float _dt) {

    g_time += _dt;

    for (auto& ease : m_eases) {
        if (!ease.finished()) { ease.update(_dt); }
    }

    m_inputHandler->update(_dt);

    m_view->update();

    {
        std::lock_guard<std::mutex> lock(m_tasksMutex);

        while (!m_tasks.empty()) {
            m_tasks.front()();
            m_tasks.pop();
        }
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

        bool updateLabels = m_labels->needUpdate();
        auto& tiles = m_tileManager->getVisibleTiles();

        if (m_view->changedOnLastUpdate() || m_tileManager->hasTileSetChanged()) {
            for (const auto& tile : tiles) {
                tile->update(_dt, *m_view);
            }
            updateLabels = true;
        }

        if (updateLabels) {
            auto& cache = m_tileManager->getTileCache();
            m_labels->update(*m_view, _dt, m_scene->styles(), tiles, cache);
        }

        bool animated = false;
        for (const auto& style : m_scene->styles()) {
            if (style->isAnimated()) {
                animated = true;
                break;
            }
        }
        if (animated != isContinuousRendering()) {
            setContinuousRendering(animated);
        }
    }
}

void render() {
    clock_t start{0};

    if (Tangram::getDebugFlag(Tangram::DebugFlags::tangram_infos)) {
        start = clock();
    }

    // Set up openGL for new frame
    RenderState::depthWrite(GL_TRUE);
    auto& color = m_scene->background();
    RenderState::clearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
        std::lock_guard<std::mutex> lock(m_tilesMutex);

        // Loop over all styles
        for (const auto& style : m_scene->styles()) {

            // Set time uniforms style's shader programs
            style->getShaderProgram()->setUniformf("u_time", g_time);

            style->onBeginDrawFrame(*m_view, *m_scene);

            // Loop over all tiles in m_tileSet
            for (const auto& tile : m_tileManager->getVisibleTiles()) {
                tile->draw(*style, *m_view);
            }

            style->onEndDrawFrame();
        }
    }

    m_labels->drawDebug(*m_view);

    if (Tangram::getDebugFlag(Tangram::DebugFlags::tangram_infos)) {

        // Force opengl to finish commands (for accurate frame time)
        glFinish();

        clock_t end = clock();
        static int cpt = 0;
        static float totaltime = 0;
        static float time = 0.f;

        // Only compute average frame time every 60 frames
        if (cpt++ > 60) {
            time = totaltime / float(cpt);
            totaltime = 0.f;
            cpt = 0;
        }

        totaltime += (float(end - start) / CLOCKS_PER_SEC) * 1000.f;

        std::vector<std::string> debuginfos;

        debuginfos.push_back("zoom:" + std::to_string(m_view->getZoom()));
        debuginfos.push_back("pos:" + std::to_string(m_view->getPosition().x) + "/"
                + std::to_string(m_view->getPosition().y));
        debuginfos.push_back("visible tiles:"
                + std::to_string(m_tileManager->getVisibleTiles().size()));
        debuginfos.push_back("tile cache size:"
                + std::to_string(m_tileManager->getTileCache()->getMemoryUsage() / 1024) + "kb");
        debuginfos.push_back("avg frame render time:" + std::to_string(time) + "ms");

        size_t memused = 0;
        for (const auto& tile : m_tileManager->getVisibleTiles()) {
            memused += tile->getMemoryUsage();
        }

        debuginfos.push_back("tile size:" + std::to_string(memused / 1024) + "kb");
        debuginfos.push_back("number of styles:"+ std::to_string(m_scene->styles().size()));

        TextDisplay::Instance().draw(debuginfos);
    }

    while (Error::hadGlError("Tangram::render()")) {}
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

void setCameraType(uint8_t _cameraType) {

    if (m_view) {
        m_view->setCameraType(static_cast<CameraType>(_cameraType));
    }
}

uint8_t getCameraType() {
    return static_cast<uint8_t>(m_view->cameraType());
}

void addDataSource(std::shared_ptr<DataSource> _source) {
    if (!m_tileManager) { return; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);

    m_tileManager->addDataSource(_source);
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



void setupGL() {

    LOG("setup GL");

    if (m_tileManager) {
        m_tileManager->clearTileSets();
    }

    // The OpenGL context has been destroyed since the last time resources were created,
    // so we invalidate all data that depends on OpenGL object handles.

    // ShaderPrograms are invalidated and immediately rebuilt
    ShaderProgram::invalidateAllPrograms();

    // Buffer objects are invalidated and re-uploaded the next time they are used
    VboMesh::invalidateAllVBOs();

    // Texture objects are invalidated and re-uploaded the next time they are updated
    Texture::invalidateAllTextures();

    // Reconfigure the render states
    RenderState::configure();

    // Set default primitive render color
    Primitives::setColor(0xffffff);

    // Load GL extensions and capabilities
    Hardware::loadExtensions();
    Hardware::loadCapabilities();

    Hardware::printAvailableExtensions();

    while (Error::hadGlError("Tangram::setupGL()")) {}
}

void runOnMainLoop(std::function<void()> _task) {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_tasks.emplace(std::move(_task));
}

}
