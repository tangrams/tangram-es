#include "sceneDirector.h"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "style/style.h"
#include "sceneDefinition/sceneDefinition.h"
#include "platform.h"

SceneDirector::SceneDirector() {

    m_tileManager.reset(TileManager::GetInstance());
    m_sceneDefinition.reset(new SceneDefinition());

    m_viewModule = std::make_shared<ViewModule>();

    m_tileManager->setView(m_viewModule);
    m_tileManager->setSceneDefiniton(m_sceneDefinition);
    m_tileManager->addDataSource(new MapzenVectorTileJson());

}

void SceneDirector::loadStyles() {

    // TODO: Instatiate styles from file
    m_sceneDefinition = std::make_shared<SceneDefinition>();

    // Create a single hard-coded style for now
    m_sceneDefinition->addStyle(std::unique_ptr<Style>(new PolygonStyle("Polygon", GL_TRIANGLES)));

}

void SceneDirector::update(float _dt) {

    m_tileManager->updateTileSet();

}

void SceneDirector::renderFrame() {

    // Set up openGL for new frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::dmat4 viewProj = m_viewModule->getViewProjectionMatrix();

    // Loop over all styles
    for (auto style : m_sceneDefinition->getStyles()) {

        style->setup();

        // Loop over visible tiles
        for (auto mapTile : m_tileManager->GetVisibleTiles()) {

            // Draw!
            mapTile->draw(style, viewProj);

        }
    }

}
