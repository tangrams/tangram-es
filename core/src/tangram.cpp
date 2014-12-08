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
#include "scene/lights.h"

namespace Tangram {

std::unique_ptr<TileManager> m_tileManager;    
std::shared_ptr<Scene> m_scene;
std::shared_ptr<View> m_view;

void initialize() {
    
    logMsg("%s\n", "initialize");

    // Create view
    m_view = std::make_shared<View>();

    // Move the view to coordinates in Manhattan so we have something interesting to test
    glm::dvec2 target = m_view->getMapProjection().LonLatToMeters(glm::dvec2(-74.00796, 40.70361));
    m_view->setPosition(target.x, target.y);

    // Load style(s); hard-coded for now
    std::unique_ptr<Style> polyStyle(new PolygonStyle("Polygon"));
    polyStyle->addLayers({
        "buildings",
        "water",
        "earth",
        "landuse"
    });
    
    std::unique_ptr<Style> linesStyle(new PolylineStyle("Polyline"));
    linesStyle->addLayers({"roads"});

    // Create a scene definition and add the style
    m_scene = std::make_shared<Scene>();
    m_scene->addStyle(std::move(polyStyle));
    m_scene->addStyle(std::move(linesStyle));

    //------ TESTING LIGHTS
    //

    //  Directional
    std::unique_ptr<DirectionalLight> directionalLight(new DirectionalLight());
    directionalLight->setDirection(glm::vec3(-1.0, -1.0, 1.0));
    //m_scene->addLight(std::move(directionalLight));
    
    //  Point
    std::unique_ptr<PointLight> pointLight(new PointLight());
    pointLight->setPosition(glm::vec3(0.0));
    m_scene->addLight(std::move(pointLight));

    //  Spot
    std::unique_ptr<SpotLight> spotLight(new SpotLight());
    spotLight->setSpecularColor(glm::vec4(0.5,0.5,0.0,1.0));
    spotLight->setPosition(glm::vec3(0.0));
    spotLight->setDirection(glm::vec3(0,PI*0.25,0.0));
    spotLight->setCutOff(PI*0.51, 2.0);
    //m_scene->addLight(std::move(spotLight));

    m_scene->injectLightning();
    //
    //-----------------------

    // Create a tileManager
    m_tileManager = TileManager::GetInstance();
    
    // Pass references to the view and scene into the tile manager
    m_tileManager->setView(m_view);
    m_tileManager->setScene(m_scene);

    // Add a tile data source
    std::unique_ptr<DataSource> dataSource(new MapzenVectorTileJson());
    m_tileManager->addDataSource(std::move(dataSource));

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

    logMsg("%s\n", "finish initialize");
}

void resize(int _newWidth, int _newHeight) {
    
    logMsg("%s\n", "resize");

    glViewport(0, 0, _newWidth, _newHeight);

    if (m_view) {
        m_view->setAspect(_newWidth, _newHeight);
    }

}

void update(float _dt) {

    if (m_tileManager) {
        m_tileManager->updateTileSet();
    }
    

    //------ TESTING LIGHTS
    //
    float time = ((float)clock())/CLOCKS_PER_SEC;
    if(m_scene->getDirectionalLights().size()){
        m_scene->getDirectionalLights()[0]->setDirection(glm::vec3(time,time*0.5,time*0.25));
    }

    if(m_scene->getPointLights().size()){
        // m_scene->getPointLights()[0]->setPosition(glm::vec3(100*cos(time),
        //                                                     100*sin(time),
        //                                                     10.0));
    }

    if(m_scene->getSpotLights().size()){
        m_scene->getSpotLights()[0]->setDirection(glm::vec3(time,time*0.5,time*0.25));
    }
    //
    //-----------------------
}

void render() {

    // Set up openGL for new frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::dmat4 view = m_view->getViewMatrix();
    glm::dmat4 viewProj = m_view->getViewProjectionMatrix();

    // Loop over all styles
    for (const auto& style : m_scene->getStyles()) {

        style->setup();

        // Loop over visible tiles
        for (const auto& mapIDandTile : m_tileManager->getVisibleTiles()) {

            const std::unique_ptr<MapTile>& tile = mapIDandTile.second;
            
            if (tile) {
                // Draw!
//                tile->draw(*style, viewProj);
                
                //  Can we pass only the scene?
                //
                tile->draw(*m_scene, *style, view, viewProj);
            }

        }
    }

    // TODO: This error checking is incomplete and only marginally useful 
    // 1. We need to continue calling glGetError until no error states remain
    // 2. Repeating an error message 60 times per second is not useful, try to consolidate 
    GLenum glError = glGetError();
    if (glError) {
        logMsg("GL Error %d!!!\n", glError);
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
    m_view->translate(-_velX * invZoomScale, _velY * invZoomScale);
    logMsg("Pan Velocity: (%f,%f)\n", _velX, _velY);
}

void handlePinchGesture(float _posX, float _posY, float _scale) {
    logMsg("Do pinch, pos1: (%f, %f)\tscale: (%f)\n", _posX, _posY, _scale);
    m_view->zoom( _scale < 1.0 ? -1 : 1);
}

void teardown() {
    // TODO: Release resources!
}
    
}
