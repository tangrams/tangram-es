#pragma once

#include <memory>

#include "tileManager/tileManager.h"
#include "viewModule/viewModule.h"
#include "util/geometryHandler.h"

class SceneDefinition;

/* Primary controller of a map view
 *
 * SceneDirector indirectly controls all aspects of the map view. 
 * Handling view information is delegated to the <ViewModule> and
 * managing map tile resources is delegated to the <TileManager>.
 * 
 * TODO: tangram.h may be an unneeded abstraction of this?
 */
class SceneDirector {

public:

    SceneDirector();

    void loadStyles();

    void onResize(int _newWidth, int _newHeight);

    void update(float _dt);

    void renderFrame();

    ~SceneDirector() {
        GeometryHandler::cleanup();
    }

private:

    std::unique_ptr<TileManager> m_tileManager;
    
    std::shared_ptr<SceneDefinition> m_sceneDefinition;
    std::shared_ptr<ViewModule> m_viewModule;

};
