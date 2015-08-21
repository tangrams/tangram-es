#include "tangram.h"

#include "platform.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "style/style.h"
#include "labels/labels.h"
#include "tile/tileManager.h"
#include "tile/tile.h"
#include "gl/error.h"
#include "gl/shaderProgram.h"
#include "scene/skybox.h"
#include "view/view.h"
#include "gl/renderState.h"
#include "gl/primitives.h"
#include "util/inputHandler.h"
#include <memory>
#include <cmath>
#include <bitset>

namespace Tangram {

std::unique_ptr<TileManager> m_tileManager;
std::shared_ptr<Scene> m_scene;
std::shared_ptr<View> m_view;
std::unique_ptr<Labels> m_labels;
std::unique_ptr<Skybox> m_skybox;
std::unique_ptr<InputHandler> m_inputHandler;

static float g_time = 0.0;
static std::bitset<8> g_flags = 0;

void initialize(const char* _scenePath) {

    logMsg("initialize\n");

    if (!m_tileManager) {

        // Create view
        m_view = std::make_shared<View>();

        // Create a scene object
        m_scene = std::make_shared<Scene>();

        // Input handler
        m_inputHandler = std::unique_ptr<InputHandler>(new InputHandler(m_view));

        m_skybox = std::unique_ptr<Skybox>(new Skybox("cubemap.png"));
        m_skybox->init();

        // Create a tileManager
        m_tileManager = TileManager::GetInstance();

        // Pass references to the view and scene into the tile manager
        m_tileManager->setView(m_view);
        m_tileManager->setScene(m_scene);

        // label setup
        m_labels = std::unique_ptr<Labels>(new Labels());

        logMsg("Loading Tangram scene file: %s\n", _scenePath);
        auto sceneString = stringFromResource(_scenePath);

        SceneLoader loader;

        loader.loadScene(sceneString, *m_scene, *m_tileManager, *m_view);

        Primitives::setColor(0xffffff);
        RenderState::configure();
    }

    while (Error::hadGlError("Tangram::initialize()")) {}

    logMsg("finish initialize\n");

}

void resize(int _newWidth, int _newHeight) {

    logMsg("resize: %d x %d\n", _newWidth, _newHeight);

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

    g_time += _dt;

    m_inputHandler->update(_dt);

    m_view->update();

    m_tileManager->updateTileSets();

    if (m_view->changedOnLastUpdate() || m_tileManager->hasTileSetChanged() || m_labels->needUpdate()) {

        auto& tiles = m_tileManager->getVisibleTiles();

        for (const auto& tile : tiles) {
            tile->update(_dt, *m_view);
        }

        m_labels->update(*m_view, _dt, m_scene->styles(), tiles);
    }

    if (m_scene) {
        // Update lights and styles
    }
}

void render() {

    // Set up openGL for new frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Loop over all styles
    for (const auto& style : m_scene->styles()) {
        style->onBeginDrawFrame(*m_view, *m_scene);

        // Loop over all tiles in m_tileSet
        for (const auto& tile : m_tileManager->getVisibleTiles()) {
            tile->draw(*style, *m_view);
        }

        style->onEndDrawFrame();
    }

    m_skybox->draw(*m_view);

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

    float screenX = _x, screenY = _y;
    m_view->screenToGroundPlane(screenX, screenY);
    glm::dvec2 meters(screenX + m_view->getPosition().x, screenY + m_view->getPosition().y);
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

void onContextDestroyed() {

    logMsg("context destroyed\n");

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
}

}
