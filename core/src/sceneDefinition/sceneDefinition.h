/*
...
*/
#pragma once

#include <vector>


/* -- Singleton Class Implementation -- */
class SceneDefinition {

public:
    // Returns a mapping of styles to VBO data
    //std::unique_ptr< std::map<std::String, std::vector<float> > > setupVBOData();

    //only one style
    /*
        Will take in geojson data, passed in from datasoure for
        the tile through the scenedirector
     */
    std::vector<float> setupVBOData();
};
