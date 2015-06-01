#include "tangram.h"

#include <memory>
#include <utility>
#include <cmath>
#include <set>

#include "platform.h"
#include "tile/tileManager.h"
#include "view/view.h"
#include "style/textStyle.h"
#include "style/debugTextStyle.h"
#include "style/debugStyle.h"
#include "style/spriteStyle.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"
#include "util/error.h"
#include "stl_util.hpp"
#include "util/tileID.h"
#include "util/skybox.h"

namespace Tangram {

    std::unique_ptr<TileManager> m_tileManager;
    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<View> m_view;
    std::shared_ptr<LabelContainer> m_labelContainer;
    std::shared_ptr<FontContext> m_ftContext;
    std::shared_ptr<DebugStyle> m_debugStyle;
    std::shared_ptr<Skybox> m_skybox;

    static float g_time = 0.0;
    static unsigned long g_flags = 0;

    void initialize() {

        logMsg("initialize\n");

        // Create view
        if (!m_view) {
            m_view = std::make_shared<View>();
        }

        // Create a scene object
        if (!m_scene) {
            m_scene = std::make_shared<Scene>();

            m_skybox = std::shared_ptr<Skybox>(new Skybox("cubemap.png"));
            m_skybox->init();
        }

        // Create a tileManager
        if (!m_tileManager) {
            m_tileManager = TileManager::GetInstance();

            // Pass references to the view and scene into the tile manager
            m_tileManager->setView(m_view);
            m_tileManager->setScene(m_scene);
        }

        // Hard-coded setup for stuff that isn't loaded through the config file yet
        m_ftContext = std::make_shared<FontContext>();
        m_ftContext->addFont("FiraSans-Medium.ttf", "FiraSans");
        m_labelContainer = LabelContainer::GetInstance();
        m_labelContainer->setFontContext(m_ftContext);
        m_labelContainer->setView(m_view);
        
        // Load scene from config file
        SceneLoader loader;
        loader.loadScene("config.yaml", *m_scene, *m_tileManager, *m_view);

        // Set up openGL state
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        glEnable(GL_DEPTH_TEST);
        glClearDepthf(1.0);
        glDepthRangef(0.0, 1.0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

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
            m_labelContainer->setScreenSize(m_view->getWidth(), m_view->getHeight());
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
                    const std::shared_ptr<MapTile>& tile = mapIDandTile.second;
                    tile->update(_dt, *m_view);
                }

                // update labels for specific style
                for (const auto& style : m_scene->getStyles()) {
                    for (const auto& mapIDandTile : m_tileManager->getVisibleTiles()) {
                        const std::shared_ptr<MapTile>& tile = mapIDandTile.second;
                        tile->updateLabels(_dt, *style, *m_view);
                    }
                }

                // manage occlusions
                m_labelContainer->updateOcclusions();

                for (const auto& style : m_scene->getStyles()) {
                    for (const auto& mapIDandTile : m_tileManager->getVisibleTiles()) {
                        const std::shared_ptr<MapTile>& tile = mapIDandTile.second;
                        tile->pushLabelTransforms(*style, m_labelContainer);
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
                const std::shared_ptr<MapTile>& tile = mapIDandTile.second;
                if (tile->hasGeometry()) {
                    // Draw tile!
                    style->onBeginDrawTile(tile);
                    tile->draw(*style, *m_view);
                }
            }

            style->onEndDrawFrame();
        }

        m_skybox->draw(*m_view);

        while (Error::hadGlError("Tangram::render()")) {}
    }

    void setViewPosition(double _lon, double _lat) {

        glm::dvec2 meters = m_view->getMapProjection().LonLatToMeters({ _lon, _lat});
        m_view->setPosition(meters.x, meters.y);

    }

    void getViewPosition(double& _lon, double& _lat) {

        glm::dvec2 meters(m_view->getPosition().x, m_view->getPosition().y);
        glm::dvec2 degrees = m_view->getMapProjection().MetersToLonLat(meters);
        _lon = degrees.x;
        _lat = degrees.y;

    }

    float getViewRotation() {

        return m_view->getRoll();

    }

    float getViewZoom() {

        return m_view->getZoom();

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

    }

    void handleDoubleTapGesture(float _posX, float _posY) {

        handlePinchGesture(_posX, _posY, 2.f);
    }

    void handlePanGesture(float _startX, float _startY, float _endX, float _endY) {

        m_view->screenToGroundPlane(_startX, _startY);
        m_view->screenToGroundPlane(_endX, _endY);

        m_view->translate(_startX - _endX, _startY - _endY);
    }

    void handlePinchGesture(float _posX, float _posY, float _scale) {

        float viewCenterX = 0.5f * m_view->getWidth();
        float viewCenterY = 0.5f * m_view->getHeight();

        m_view->screenToGroundPlane(viewCenterX, viewCenterY);
        m_view->screenToGroundPlane(_posX, _posY);

        m_view->translate((_posX - viewCenterX)*(1-1/_scale), (_posY - viewCenterY)*(1-1/_scale));

        m_view->zoom(log2f(_scale));
    }

    void handleRotateGesture(float _posX, float _posY, float _radians) {

        m_view->screenToGroundPlane(_posX, _posY);
        m_view->orbit(_posX, _posY, _radians);

    }

    void handleShoveGesture(float _distance) {

        m_view->pitch(_distance);

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

    void teardown() {
        // Release resources!
        logMsg("teardown\n");
        m_tileManager.reset();
        m_scene.reset();
        m_view.reset();
    }

    void onContextDestroyed() {

        logMsg("context destroyed\n");

        // The OpenGL context has been destroyed since the last time resources were created,
        // so we invalidate all data that depends on OpenGL object handles.

        // ShaderPrograms are invalidated and immediately rebuilt
        ShaderProgram::invalidateAllPrograms();

        // Buffer objects are invalidated and re-uploaded the next time they are used
        VboMesh::invalidateAllVBOs();

    }

}

