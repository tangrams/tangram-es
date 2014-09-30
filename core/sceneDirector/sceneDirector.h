#pragma once

#include <vector>
#include "tileManager/tileManager.h"
#include "sceneDefinition/sceneDefinition.h"
#include "viewModule/viewModule.h"

class SceneDirector {

public:

    SceneDirector();

    void loadStyles();

    void update(float _dt);

    void renderFrame();

private:

    std::unique_ptr<TileManager> m_tileManager;
    std::unique_ptr<SceneDefinition> m_sceneDefinition;
    
    std::shared_ptr<ViewModule> m_viewModule;

};
