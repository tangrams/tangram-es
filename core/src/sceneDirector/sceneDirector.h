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
    
    /*
     * Tap centers the view on the tap position
     */
    void onTap(const glm::vec2& _point);
    
    /*
     * NoOP for doubleTap
     * TODO: use it to zoom-in/out in the same tile
     */
    void onDoubleTap(const glm::vec2& _point);

    /*
     * Pan's the view by using the velocity of the pan
     * normalize the velocity and translate the view with a scaled down value of the normalized velocity
     * TODO: Take zoom level into account while panning
     */
    void onPan(const glm::vec2& _velocity);

    /*
     * Naive tile zoom up/down based on descete pinch/scale values
     * TODO: continous zooming
     */
    void onPinch(const glm::vec2& _position1, const float& _scale);
    
    void update(float _dt);

    void renderFrame();

    ~SceneDirector() {}

private:

    std::unique_ptr<TileManager> m_tileManager;
    
    std::shared_ptr<SceneDefinition> m_sceneDefinition;
    std::shared_ptr<ViewModule> m_viewModule;

};
