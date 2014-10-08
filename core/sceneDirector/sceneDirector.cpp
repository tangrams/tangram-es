#include "sceneDirector.h"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "style/style.h"
#include "sceneDefinition/sceneDefinition.h"
#include "platform.h"

SceneDirector::SceneDirector() {

    m_viewModule = std::make_shared<ViewModule>();
    glm::dvec2 target = m_viewModule->getMapProjection().LonLatToMeters(glm::dvec2(-74.00796, 40.70361));
    m_viewModule->setPosition(target.x, target.y);

    logMsg("Constructed viewModule\n");

    m_tileManager = TileManager::GetInstance();
    m_tileManager->setView(m_viewModule);
    std::shared_ptr<DataSource> dataSource(new MapzenVectorTileJson());
    m_tileManager->addDataSource(std::move(dataSource));

    logMsg("Constructed tileManager \n");
}

void SceneDirector::loadStyles() {

    // TODO: Instatiate styles from file
    m_sceneDefinition = std::make_shared<SceneDefinition>();

    // Create hard-coded styles for now
    std::unique_ptr<Style> style(new PolygonStyle("Polygon"));
    style->updateLayers({{"water", glm::vec4(0.1, 0.1, 1.0, 1.0)}});
    style->updateLayers({{"buildings", glm::vec4(0.7, 0.7, 0.7, 1.0)}});
    style->updateLayers({{"earth", glm::vec4(0.1, 0.9, 0.1, 1.0)}});
    style->updateLayers({{"landuse", glm::vec4(0.1, 0.6, 0.1, 1.0)}});
    m_sceneDefinition->addStyle(std::move(style));

    m_tileManager->setSceneDefinition(m_sceneDefinition);

    logMsg("Loaded styles\n");

}

void SceneDirector::update(float _dt) {

    m_tileManager->updateTileSet();

}

void SceneDirector::renderFrame() {

    // Set up openGL for new frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::dmat4 viewProj = m_viewModule->getViewProjectionMatrix();

    // Loop over all styles
    for (const auto& style : m_sceneDefinition->getStyles()) {

        style->setup();

        // Loop over visible tiles
        for (const auto& mapTile : m_tileManager->getVisibleTiles()) {

            // Draw!
            mapTile.second->draw(*style, viewProj);

        }
    }

}
