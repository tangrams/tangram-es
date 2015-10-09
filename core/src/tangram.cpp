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
#include "util/inputHandler.h"
#include "view/view.h"
#include "data/clientGeoJsonSource.h"
#include "gl.h"
#include "gl/extension.h"
#include <memory>
#include <cmath>
#include <bitset>
#include <mutex>

namespace Tangram {

#ifdef USE_REMOTERY
struct rmtDeleter {
   void operator()(Remotery *rmt) const {
       rmt_DestroyGlobalInstance(rmt);
   }
};

static std::unique_ptr<Remotery, rmtDeleter> rmt = nullptr;
#endif

std::unique_ptr<TileManager> m_tileManager;
std::shared_ptr<Scene> m_scene;
std::shared_ptr<View> m_view;
std::unique_ptr<Labels> m_labels;
std::unique_ptr<Skybox> m_skybox;
std::unique_ptr<InputHandler> m_inputHandler;
std::mutex m_tilesMutex;

static float g_time = 0.0;
static std::bitset<8> g_flags = 0;
int log_level = 2;

void initialize(const char* _scenePath) {

#ifdef USE_REMOTERY
    Remotery* _rmt = nullptr;
    rmt_CreateGlobalInstance(&_rmt);
    rmt = std::unique_ptr<Remotery, rmtDeleter>(_rmt);
    rmt_SetCurrentThreadName("MainThread");
#endif

    if (m_tileManager) {
        LOG("Notice: Already initialized");
        return;
    }

    LOG("initialize");

    auto sceneRelPath = setResourceRoot(_scenePath);

    // Create view
    m_view = std::make_shared<View>();

    // Create a scene object
    m_scene = std::make_shared<Scene>();

    // Input handler
    m_inputHandler = std::unique_ptr<InputHandler>(new InputHandler(m_view));

    // Create a tileManager
    m_tileManager = TileManager::GetInstance();

    // Pass references to the view and scene into the tile manager
    m_tileManager->setView(m_view);

    // label setup
    m_labels = std::unique_ptr<Labels>(new Labels());

    LOG("Loading Tangram scene file: %s", sceneRelPath.c_str());
    auto sceneString = stringFromFile(sceneRelPath.c_str(), PathType::resource);

    if (SceneLoader::loadScene(sceneString, *m_scene)) {
        // To add font for debugTextStyle
        m_scene->fontContext()->addFont("FiraSans", "Medium", "");

        m_tileManager->setScene(m_scene);

        glm::dvec2 projPos = m_view->getMapProjection().LonLatToMeters(m_scene->startPosition);
        m_view->setPosition(projPos.x, projPos.y);
        m_view->setZoom(m_scene->startZoom);
    }

    LOG("finish initialize");

}

void loadScene(const char* _scenePath) {
    LOG("Loading scene file: %s", _scenePath);

    auto sceneString = stringFromFile(setResourceRoot(_scenePath).c_str(), PathType::resource);

    auto scene = std::make_shared<Scene>();
    if (SceneLoader::loadScene(sceneString, *scene)) {
        m_scene = scene;
        m_scene->fontContext()->addFont("FiraSans", "Medium", "");

        m_tileManager->setScene(scene);
    }
}

void resize(int _newWidth, int _newHeight) {

    LOG("resize: %d x %d", _newWidth, _newHeight);

    glViewport(0, 0, _newWidth, _newHeight);

    if (m_view) {
        m_view->setSize(_newWidth, _newHeight);
    }

    for (auto& style : m_scene->styles()) {
        style->viewportHasChanged();
    }

    Primitives::setResolution(_newWidth, _newHeight);

    while (Error::hadGlError("Tangram::resize()")) {}

}

void update(float _dt) {
    RMT_Sample(update);

    g_time += _dt;

    m_inputHandler->update(_dt);

    {
        RMT_Sample(updateView);
        m_view->update();
    }

    {
        std::lock_guard<std::mutex> lock(m_tilesMutex);

        RMT_BeginSample(updateTileSets);
        m_tileManager->updateTileSets();
        RMT_EndSample();

        if (m_view->changedOnLastUpdate() || m_tileManager->hasTileSetChanged() || m_labels->needUpdate()) {

            auto& tiles = m_tileManager->getVisibleTiles();

            for (const auto& tile : tiles) {
                tile->update(_dt, *m_view);
            }

            m_labels->update(*m_view, _dt, m_scene->styles(), tiles);
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
    RMT_Sample(render);

    RMT_BeginSample(clear);

// Set up openGL for new frame
    RenderState::depthWrite(GL_TRUE);
    auto& color = m_scene->background();
    RenderState::clearColor(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RMT_EndSample();

    {
        std::lock_guard<std::mutex> lock(m_tilesMutex);

        // Loop over all styles
        for (const auto& style : m_scene->styles()) {
            RMT_BeginSample(style);

            // Set time uniforms style's shader programs
            style->getShaderProgram()->setUniformf("u_time", g_time);

            style->onBeginDrawFrame(*m_view, *m_scene);

            // Loop over all tiles in m_tileSet
            for (const auto& tile : m_tileManager->getVisibleTiles()) {
                RMT_Sample(tile);

                tile->draw(*style, *m_view);
            }

            style->onEndDrawFrame();

            RMT_EndSample();
        }
    }

    m_labels->drawDebug(*m_view);

    while (Error::hadGlError("Tangram::render()")) {}
}

void setPosition(double _lon, double _lat) {

    glm::dvec2 meters = m_view->getMapProjection().LonLatToMeters({ _lon, _lat});
    m_view->setPosition(meters.x, meters.y);
    requestRender();

}

void getPosition(double& _lon, double& _lat) {

    glm::dvec2 meters(m_view->getPosition().x, m_view->getPosition().y);
    glm::dvec2 degrees = m_view->getMapProjection().MetersToLonLat(meters);
    _lon = degrees.x;
    _lat = degrees.y;

}

void setZoom(float _z) {

    m_view->setZoom(_z);
    requestRender();

}

float getZoom() {

    return m_view->getZoom();

}

void setRotation(float _radians) {

    m_view->setRoll(_radians);
    requestRender();

}

float getRotation() {

    return m_view->getRoll();

}


void setTilt(float _radians) {

    m_view->setPitch(_radians);
    requestRender();

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

int addDataSource(const char* _name) {

    if (!m_tileManager) { return -1; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    auto source = std::make_shared<ClientGeoJsonSource>(std::string(_name), "");
    m_tileManager->addDataSource(source);

    return source->id();
}

void clearSourceData(int _sourceId) {

    if (!m_tileManager) { return; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    for (auto& set : m_tileManager->getTileSets()) {
        if (set.source->id() == _sourceId) {
            set.source->clearData();
            m_tileManager->clearTileSet(_sourceId);
        }
    }
    requestRender();
}

void addSourcePoint(int _sourceId, double* _coords) {
    if (!m_tileManager) { return; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    auto source = m_tileManager->getClientSourceById(_sourceId);
    if (source) {
        source->addPoint(_coords);
        m_tileManager->clearTileSet(_sourceId);
    }
    requestRender();
}

void addSourceLine(int _sourceId, double* _coords, int _lineLength) {
    if (!m_tileManager) { return; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    auto source = m_tileManager->getClientSourceById(_sourceId);
    if (source) {
        source->addLine(_coords, _lineLength);
        m_tileManager->clearTileSet(_sourceId);
    }
    requestRender();
}

void addSourcePoly(int _sourceId, double* _coords, int* _ringLengths, int _rings) {
    if (!m_tileManager) { return; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    auto source = m_tileManager->getClientSourceById(_sourceId);
    if (source) {
        source->addPoly(_coords, _ringLengths, _rings);
        m_tileManager->clearTileSet(_sourceId);
    }
    requestRender();
}

void addSourceGeoJSON(int _sourceId, const char* _data) {
    if (!m_tileManager) { return; }
    std::lock_guard<std::mutex> lock(m_tilesMutex);
    auto source = m_tileManager->getClientSourceById(_sourceId);
    if (source) {
        source->addData(_data);
        m_tileManager->clearTileSet(_sourceId);
    }
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

        for (auto& style : m_scene->styles()) {
           style->notifyGLContextLost();
        }
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

    // Load GL extensions
    GLExtensions::load();

    GLExtensions::printAvailableExtensions();

    while (Error::hadGlError("Tangram::setupGL()")) {}
}

}
