#pragma once

#include <vector>
#include "viewModule.h"

class TileManager;
class SceneDefinition;
class ViewModule;

/* -- Singleton Class Implementation -- */
class SceneDirector {
    TileManager *m_tileManager;
    SceneDefinition *m_sceneDefinition;
    ViewModule *m_viewModule;
    SceneDirector();

public:
    /*
        Note: UpdateVBO iterates through all the visible tiles,
        and "moves" (unique pointer concepts) the vbo data created
        in the sceneDefinition into the appropriate tile style vbo..

        This will pass the geoJson data from the data source to the
        sceneDefinition instance, which will construct the vbo data.
    */
    bool updateVBOs();
    bool renderFrame(float dt);
};
