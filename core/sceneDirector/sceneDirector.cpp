#include "sceneDirector.h"

SceneDirector::SceneDirector() {

    m_tileManager.reset(TileManager::GetInstance());
    m_sceneDefinition.reset(new SceneDefinition());

    m_viewModule = std::make_shared<ViewModule>();

    m_tileManager->setView(m_viewModule);
    m_tileManager->addDataSource(new MapzenVectorTileJson());

}

void SceneDirector::loadStyles() {

    // TODO: Instatiate styles from file

}

void SceneDirector::update(float _dt) {

    m_tileManager->updateTileSet(m_sceneDefinition->getStyles());

}

void SceneDirector::renderFrame() {

    // Set up openGL for new frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 viewProj = m_viewModule->getViewProjectionMatrix();

    // Loop over all styles
    for (auto style : m_sceneDefinition->getStyles()) {

        style->setup();

        // Loop over visible tiles
        for (auto mapTile : m_tileManager->GetVisibleTiles()) {

            // Draw!
            mapTile->draw(viewProj, style);

        }
    }

}
