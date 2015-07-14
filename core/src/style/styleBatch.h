 #pragma once

#include "styleParamMap.h"

#include "glm/fwd.hpp"

class View;
class Feature;
class MapTile;

class StyleBatch {
public:

    /*** Methods called on render thread ***/

    /* Draw Batch */
    virtual void draw(const View& _view) = 0;

    /* Update state of Batch objects for the current View */
    virtual void update(const glm::mat4& mvp, const View& _view, float _dt) = 0;

    /* Prepare updated data for rendering */
    virtual void prepare() = 0;


    /*** Methods called on TileWorker during creation ***/

    /* Add <Feature> to this <Batch> and apply styling for rendering */
    virtual void add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) = 0;

    virtual bool compile() = 0;
};
