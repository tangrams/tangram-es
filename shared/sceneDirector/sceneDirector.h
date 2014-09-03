/*
...
*/

#ifndef __SCENE_DIRECTOR_H__
#define __SCENE_DIRECTOR_H__

#include <vector>

class TileManager;
class SceneDefinition;

/* -- Singleton Class Implementation -- */
class SceneDirector {
    TileManager *m_TileManager;
    SceneDefinition *m_SceneDefinition;
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
    bool renderLoop();
};

#endif