#include "tangram.h"

#include <memory>
#include <utility>
#include <cmath>

#include "platform.h"
#include "tile/tileManager.h"
#include "view/view.h"
#include "data/dataSource.h"

#include "style/polygonStyle.h"
#include "style/polylineStyle.h"
#include "scene/scene.h"
#include "util/error.h"

namespace Tangram {

std::unique_ptr<TileManager> m_tileManager;    
std::shared_ptr<Scene> m_scene;
std::shared_ptr<View> m_view;

void initialize() {
    
    logMsg("%s\n", "initialize");

    // Create view
    if (!m_view) {
        m_view = std::make_shared<View>();
        
        // Move the view to coordinates in Manhattan so we have something interesting to test
        glm::dvec2 target = m_view->getMapProjection().LonLatToMeters(glm::dvec2(-74.00796, 40.70361));
        m_view->setPosition(target.x, target.y);
    }

    // Create a scene object
    if (!m_scene) {
        m_scene = std::make_shared<Scene>();
        
        // Load style(s); hard-coded for now
        std::unique_ptr<Style> polyStyle(new PolygonStyle("Polygon"));
        polyStyle->addLayers({
            "buildings",
            "water",
            "earth",
            "landuse"
        });
        m_scene->addStyle(std::move(polyStyle));
        
        std::unique_ptr<Style> linesStyle(new PolylineStyle("Polyline"));
        linesStyle->addLayers({"roads"});
        m_scene->addStyle(std::move(linesStyle));
    }

    // Create a tileManager
    if (!m_tileManager) {
        m_tileManager = TileManager::GetInstance();
        
        // Pass references to the view and scene into the tile manager
        m_tileManager->setView(m_view);
        m_tileManager->setScene(m_scene);
        
        // Add a tile data source
        std::unique_ptr<DataSource> dataSource(new MapzenVectorTileJson());
        m_tileManager->addDataSource(std::move(dataSource));
    }

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

    logMsg("%s\n", "finish initialize");

}

void resize(int _newWidth, int _newHeight) {
    
    logMsg("resize: %d x %d\n", _newWidth, _newHeight);

    glViewport(0, 0, _newWidth, _newHeight);

    if (m_view) {
        m_view->setSize(_newWidth, _newHeight);
    }

    while (Error::hadGlError("Tangram::resize()")) {}

}

void update(float _dt) {

    if (m_view) {
        m_view->update();
    }
    
    if (m_tileManager) {
        m_tileManager->updateTileSet();
    }

}

void render() {

    // Set up openGL for new frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::dmat4 viewProj = m_view->getViewProjectionMatrix();

    // Loop over all styles
    for (const auto& style : m_scene->getStyles()) {

        style->setup();

        // Loop over visible tiles
        for (const auto& mapIDandTile : m_tileManager->getVisibleTiles()) {

            const std::shared_ptr<MapTile>& tile = mapIDandTile.second;
            if (tile) {
                // Draw!
                tile->draw(*style, viewProj);
            }
        }
        
        // Loop over proxy tiles
        for(const auto& mapIDandTile : m_tileManager->getProxyTiles()) {
            const std::shared_ptr<MapTile>& tile = mapIDandTile.second;
            if(tile) {
                // Draw!
                logMsg("Drawing Proxy Tile: [%d, %d, %d]\n", mapIDandTile.first.z, mapIDandTile.first.x, mapIDandTile.first.y);
                tile->draw(*style, viewProj);
            }
        }
        
    }

    while (Error::hadGlError("Tangram::render()")) {}

}

void setPixelScale(float _pixelsPerPoint) {
    
    if (m_view) {
        m_view->setPixelScale(_pixelsPerPoint);
    }
    
}
    
void handleTapGesture(float _posX, float _posY) {
    logMsg("Do tap: (%f,%f)\n", _posX, _posY);
    m_view->translate(_posX, _posY);
}

void handleDoubleTapGesture(float _posX, float _posY) {
    logMsg("Do double tap: (%f,%f)\n", _posX, _posY);
}

void handlePanGesture(float _velX, float _velY) {
    // Scaled with reference to 16 zoom level
    float invZoomScale = 0.1 * pow(2,(16 - m_view->getZoom()));
    m_view->translate(-_velX * invZoomScale * 2.0, _velY * invZoomScale * 2.0);
    logMsg("Pan Velocity: (%f,%f)\n", _velX, _velY);
}

void handlePinchGesture(float _posX, float _posY, float _scale) {
    logMsg("Do pinch, pos1: (%f, %f)\tscale: (%f)\n", _posX, _posY, _scale);
    if(_scale < 1.0) {
        m_view->zoom((_scale - 1.0)*0.25);
    }
    else if(_scale > 1.0 && _scale < 10.0) {
        m_view->zoom((_scale - (int)_scale)*0.25);
    }
}

void teardown() {
    // TODO: Release resources!
}

void onContextDestroyed() {
    
    // The OpenGL context has been destroyed since the last time resources were created,
    // so we invalidate all data that depends on OpenGL object handles.

    // ShaderPrograms are invalidated and immediately rebuilt
    ShaderProgram::invalidateAllPrograms();

    // Buffer objects are invalidated and re-uploaded the next time they are used
    VboMesh::invalidateAllVBOs();
    
}
    
}
