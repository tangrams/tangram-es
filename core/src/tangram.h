#pragma once

#include "glm/glm.hpp"

#include "platform.h"

/* Tangram API
 *
 * Primary interface for controlling and managing the lifecycle of a Tangram map surface
 * 
 * TODO: More complete lifecycle management such as onPause, onResume
 * TODO: Input functions will go here too, things like onTouchDown, onTouchMove, etc.
 */

namespace Tangram {

// Create resources and initialize the map view
void initialize();

// Resize the map view to a new width and height (in pixels)
void resize(int _newWidth, int _newHeight);

// Update the map state with the time interval since the last update
void update(float _dt);

// Render a new frame of the map view (if needed)
void render();

// Respond to touch input
void handleTapGestures(const glm::vec2 _position);
    
void handleDoubleTapGestures(const glm::vec2 _position);
    
void handlePanGestures(const glm::vec2 _velocity);
    
void handlePinchGestures(const glm::vec2 _position, const float _scale = 1.0);

// Release resources and shut down renderer
void teardown();

}

