#include "tangram.h"

#include "platform.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "style/style.h"
#include "text/fontContext.h"
#include "labels/labels.h"
#include "tile/tileManager.h"
#include "tile/tile.h"
#include "gl/error.h"
#include "gl/shaderProgram.h"
#include "scene/skybox.h"
#include "view/view.h"
#include "gl/renderState.h"
#include <memory>
#include <cmath>

namespace Tangram {

    std::unique_ptr<TileManager> m_tileManager;
    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<View> m_view;
    std::shared_ptr<Labels> m_labels;
    std::shared_ptr<FontContext> m_ftContext;
    std::shared_ptr<Skybox> m_skybox;

    static float g_time = 0.0;
    static unsigned long g_flags = 0;

    void initialize() {

        logMsg("initialize\n");

        if (!m_tileManager) {

            // Create view
            m_view = std::make_shared<View>();

            // Create a scene object
            m_scene = std::make_shared<Scene>();

            m_skybox = std::shared_ptr<Skybox>(new Skybox("cubemap.png"));
            m_skybox->init();

            // Create a tileManager
            m_tileManager = TileManager::GetInstance();

            // Pass references to the view and scene into the tile manager
            m_tileManager->setView(m_view);
            m_tileManager->setScene(m_scene);

            // Font and label setup
            m_ftContext = std::make_shared<FontContext>();
            m_ftContext->addFont("FiraSans-Medium.ttf", "FiraSans");
            m_labels = Labels::GetInstance();
            m_labels->setFontContext(m_ftContext);
            m_labels->setView(m_view);

            SceneLoader loader;
            loader.loadScene("config.yaml", *m_scene, *m_tileManager, *m_view);

        }

        RenderState::configure();

        while (Error::hadGlError("Tangram::initialize()")) {}

        logMsg("finish initialize\n");

    }

    void resize(int _newWidth, int _newHeight) {

        logMsg("resize: %d x %d\n", _newWidth, _newHeight);

        glViewport(0, 0, _newWidth, _newHeight);

        if (m_view) {
            m_view->setSize(_newWidth, _newHeight);
        }

        if (m_ftContext) {
            m_ftContext->setScreenSize(m_view->getWidth(), m_view->getHeight());
            m_labels->setScreenSize(m_view->getWidth(), m_view->getHeight());
        }

        while (Error::hadGlError("Tangram::resize()")) {}

    }

    void update(float _dt) {

        g_time += _dt;

        if (m_view) {

            m_view->update();

            m_tileManager->updateTileSet();

            if(m_view->changedOnLastUpdate() || m_tileManager->hasTileSetChanged() || Label::s_needUpdate) {
                Label::s_needUpdate = false;

                for (const auto& mapIDandTile : m_tileManager->getVisibleTiles()) {
                    const auto& tile = mapIDandTile.second;
                    if (tile->isReady()) {
                        tile->update(_dt, *m_view);
                    }
                }

                // update labels for specific style
                for (const auto& style : m_scene->getStyles()) {
                    for (const auto& mapIDandTile : m_tileManager->getVisibleTiles()) {
                        const auto& tile = mapIDandTile.second;
                        if (tile->isReady()) {
                            tile->updateLabels(_dt, *style, *m_view);
                        }
                    }
                }

                // manage occlusions
                m_labels->updateOcclusions();

                for (const auto& style : m_scene->getStyles()) {
                    for (const auto& mapIDandTile : m_tileManager->getVisibleTiles()) {
                        const auto& tile = mapIDandTile.second;
                        if (tile->isReady()) {
                            tile->pushLabelTransforms(*style, m_labels);
                        }
                    }
                }
            }

            if (Label::s_needUpdate) {
                requestRender();
            }
        }

        if(m_scene) {
            // Update lights and styles
        }
    }

    void render() {

        // Set up openGL for new frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Loop over all styles
        for (const auto& style : m_scene->getStyles()) {
            style->onBeginDrawFrame(m_view, m_scene);

            // Loop over all tiles in m_tileSet
            for (const auto& mapIDandTile : m_tileManager->getVisibleTiles()) {
                const std::shared_ptr<Tile>& tile = mapIDandTile.second;
                if (tile->isReady()) {
                    // Draw tile!
                    tile->draw(*style, *m_view);
                }
            }

            style->onEndDrawFrame();
        }

        m_skybox->draw(*m_view);

        m_labels->drawDebug();

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

        for (auto& style : m_scene->getStyles()) {
            style->setPixelScale(_pixelsPerPoint);
        }

    }

    void handleTapGesture(float _posX, float _posY) {

        float viewCenterX = 0.5f * m_view->getWidth();
        float viewCenterY = 0.5f * m_view->getHeight();

        m_view->screenToGroundPlane(viewCenterX, viewCenterY);
        m_view->screenToGroundPlane(_posX, _posY);

        m_view->translate((_posX - viewCenterX), (_posY - viewCenterY));

        requestRender();
    }

    void handleDoubleTapGesture(float _posX, float _posY) {

        handlePinchGesture(_posX, _posY, 2.f);
    }

    void handlePanGesture(float _startX, float _startY, float _endX, float _endY) {

        m_view->screenToGroundPlane(_startX, _startY);
        m_view->screenToGroundPlane(_endX, _endY);

        m_view->translate(_startX - _endX, _startY - _endY);

        requestRender();
    }

    void handlePinchGesture(float _posX, float _posY, float _scale) {

        float viewCenterX = 0.5f * m_view->getWidth();
        float viewCenterY = 0.5f * m_view->getHeight();

        m_view->screenToGroundPlane(viewCenterX, viewCenterY);
        m_view->screenToGroundPlane(_posX, _posY);

        m_view->translate((_posX - viewCenterX)*(1-1/_scale), (_posY - viewCenterY)*(1-1/_scale));

        static float invLog2 = 1 / log(2);
        m_view->zoom(log(_scale) * invLog2);

        requestRender();
    }

    void handleRotateGesture(float _posX, float _posY, float _radians) {

        m_view->screenToGroundPlane(_posX, _posY);
        m_view->orbit(_posX, _posY, _radians);

        requestRender();
    }

    void handleShoveGesture(float _distance) {

        m_view->pitch(_distance);

        requestRender();
    }

    void setDebugFlag(DebugFlags _flag, bool _on) {

        if (_on) {
            g_flags |= (1 << _flag); // |ing with a bitfield that is 0 everywhere except index _flag; sets index _flag to 1
        } else {
            g_flags &= ~(1 << _flag); // &ing with a bitfield that is 1 everywhere except index _flag; sets index _flag to 0
        }

        m_view->setZoom(m_view->getZoom()); // Force the view to refresh

    }

    bool getDebugFlag(DebugFlags _flag) {

        return (g_flags & (1 << _flag)) != 0; // &ing with a bitfield that is 0 everywhere except index _flag will yield 0 iff index _flag is 0

    }

    void onContextDestroyed() {

        logMsg("context destroyed\n");

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

