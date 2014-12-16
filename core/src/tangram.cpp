#include "tangram.h"

#include <memory>
#include <utility>
#include <cmath>
#include <time.h>

#include "platform.h"
#include "tile/tileManager.h"
#include "view/view.h"
#include "data/dataSource.h"

#include "style/polygonStyle.h"
#include "style/polylineStyle.h"
#include "scene/scene.h"
#include "scene/lights.h"
#include "util/error.h"

namespace Tangram {

std::unique_ptr<TileManager> m_tileManager;    
std::shared_ptr<Scene> m_scene;
std::shared_ptr<View> m_view;

static float g_time = 0.0;

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

        //------ TESTING LIGHTS

        //  Directional
        DirectionalLight* dlight = new DirectionalLight("dLight");
        dlight->setDiffuseColor(glm::vec4(0.3,0.2,0.0,1.0));
        dlight->setDirection(glm::vec3(-1.0, -1.0, 1.0));
        std::shared_ptr<Light> directionalLight(dlight);
        m_scene->addLight(directionalLight);
    
        // //  Point
        PointLight * pLight = new PointLight("pLight",true);
        pLight->setDiffuseColor(glm::vec4(0.0,1.0,0.0,1.0));
        pLight->setSpecularColor(glm::vec4(0.5,0.0,1.0,1.0));
        pLight->setLinearAttenuation(0.005);
        pLight->setPosition(glm::vec3(0.0));
        std::shared_ptr<Light> pointLight(pLight);
        m_scene->addLight(pointLight);

        // // //  Spot
        SpotLight * sLight = new SpotLight("sLight",true);
        sLight->setSpecularColor(glm::vec4(0.5,0.5,0.0,1.0));
        sLight->setPosition(glm::vec3(0.0));
        sLight->setDirection(glm::vec3(0,PI*0.25,0.0));
        sLight->setCutOff(PI*0.1, 20.0);
        std::shared_ptr<Light> spotLight(sLight);
        m_scene->addLight(spotLight);
        
        //-----------------------

        m_scene->buildShaders();
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

    g_time += _dt;

    if (m_view) {
        m_view->update();
    }
    
    if (m_tileManager) {
        m_tileManager->updateTileSet();
    }
    
    if(m_scene){
        for(int i = 0 ; i < m_scene->getLights().size(); i++){

            if(m_scene->getLights()[i]->getType() == LightType::LIGHT_DIRECTIONAL){
                DirectionalLight* tmp = dynamic_cast<DirectionalLight*>( (m_scene->getLights()[i]).get() );
                // tmp->setDirection(glm::vec3(0.0, sin(g_time), 1.0));
            } else if(m_scene->getLights()[i]->getType() == LightType::LIGHT_POINT){
                PointLight* tmp = dynamic_cast<PointLight*>( (m_scene->getLights()[i]).get() );
                tmp->setPosition(glm::vec3( 200*cos(g_time*0.8),
                                            200*sin(g_time*0.3), 
                                            -m_view->getPosition().z+100));
            } else if(m_scene->getLights()[i]->getType() == LightType::LIGHT_SPOT){
                SpotLight* tmp = dynamic_cast<SpotLight*>( (m_scene->getLights()[i]).get() );
                tmp->setDirection(glm::vec3(cos(g_time),
                                            sin(g_time), 
                                            0.0));
                tmp->setPosition(glm::vec3(0.0, 0.0, -m_view->getPosition().z+100));
            } 
        }
    }   
}

void render() {

    // Set up openGL for new frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
                tile->draw(*m_scene, *style, *m_view);
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
    logMsg("Tap: (%f,%f)\n", _posX, _posY);

}

void handleDoubleTapGesture(float _posX, float _posY) {
    
    logMsg("Double tap: (%f,%f)\n", _posX, _posY);

}

void handlePanGesture(float _dX, float _dY) {
    
    float dx = m_view->toWorldDistance(_dX);
    float dy = m_view->toWorldDistance(_dY);

    // We flip the signs of dx and dy to move the camera in the opposite direction
    // of the intended "world movement", but dy gets flipped once more because screen
    // coordinates have y pointing down and our world coordinates have y pointing up
    m_view->translate(-dx, dy);
    logMsg("Drag: (%f,%f)\n", _dX, _dY);

}

void handlePinchGesture(float _posX, float _posY, float _scale) {
    
    logMsg("Pinch: (%f, %f)\tscale: (%f)\n", _posX, _posY, _scale);
    m_view->zoom( _scale < 1.0 ? -1 : 1);

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
