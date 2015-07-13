 #pragma once

#include "styleParamMap.h"

#include "glm/fwd.hpp"

class View;
class Feature;
class MapTile;

class StyleBatch {
public:
    virtual void draw(const View& _view) = 0;
    virtual void update(const glm::mat4& mvp, const View& _view, float _dt) = 0;
    virtual void prepare() = 0;
    virtual bool compile() = 0;

    virtual void add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) = 0;

    /* Perform any needed setup to process the data for a tile */
    virtual void onBeginBuildTile() {};

    /* Perform any needed teardown after processing data for a tile */
    virtual void onEndBuildTile() {};
};
