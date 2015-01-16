#include "tangram.h"

#include <memory>
#include <utility>
#include <cmath>
#include <set>

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
    
    logMsg("initialize\n");

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

    logMsg("finish initialize\n");

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


        // Loop over all tiles in m_tileSet
        for (const auto& mapIDandTile : m_tileManager->getVisibleTiles()) {
            const std::shared_ptr<MapTile>& tile = mapIDandTile.second;
            if (tile->hasGeometry()) {
                if (tile->getProxyCounter() > 0) {
                    // Draw proxy tiles:
                    // each proxy tile is drawn at a specific depth plane depending on the zoom level of the tile
                    // higher zoom level are drawn above
                    style->setup(1.0f + log( (m_view->s_maxZoom+1)/(m_view->s_maxZoom + 1 - tile->getID().z)));
                    logMsg("Proxy: (%d, %d, %d)\n", tile->getID().x, tile->getID().y, tile->getID().z);
                    tile->draw(*style, viewProj);
                } else {
                    // Draw visible tile:
                    // always drawn in the same z place above all proxy tiles
                    style->setup(1.0f + log(m_view->s_maxZoom+2));
                    tile->draw(*style, viewProj);
                }
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
    
    float dx = m_view->toWorldDistance(_posX - 0.5 * m_view->getWidth());
    float dy = m_view->toWorldDistance(_posY - 0.5 * m_view->getHeight());

    // Flip y displacement to change from screen coordinates to world coordinates
    m_view->translate(dx, -dy);
    

}

void handleDoubleTapGesture(float _posX, float _posY) {
    
    m_view->zoom(1.0);
}

void handlePanGesture(float _dX, float _dY) {
    
    float dx = m_view->toWorldDistance(_dX);
    float dy = m_view->toWorldDistance(_dY);

    // We flip the signs of dx and dy to move the camera in the opposite direction
    // of the intended "world movement", but dy gets flipped once more because screen
    // coordinates have y pointing down and our world coordinates have y pointing up
    m_view->translate(-dx, dy);

}

void handlePinchGesture(float _posX, float _posY, float _scale) {
    m_view->zoom(log2f(_scale));
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
