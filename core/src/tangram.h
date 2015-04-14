#pragma once

#include "platform.h"
#include "debug.h"

/* Tangram API
 *
 * Primary interface for controlling and managing the lifecycle of a Tangram map surface
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

    // Release resources and shut down renderer
    void teardown();

    // Invalidate and re-create all OpenGL resources
    void onContextDestroyed();

    // Set the ratio of hardware pixels to logical pixels (defaults to 1.0)
    void setPixelScale(float _pixelsPerPoint);

    // Respond to a tap at the given screen coordinates (x right, y down)
    void handleTapGesture(float _posX, float _posY);

    // Respond to a double tap at the given screen coordinates (x right, y down)
    void handleDoubleTapGesture(float _posX, float _posY);

    // Respond to a drag with the given displacement in screen coordinates (x right, y down)
    void handlePanGesture(float _startX, float _startY, float _endX, float _endY);

    // Respond to a pinch at the given position in screen coordinates with the given incremental scale
    void handlePinchGesture(float _posX, float _posY, float _scale);

    // Respond to a rotation gesture with the given incremental rotation in radians
    void handleRotateGesture(float _posX, float _posY, float _rotation);

    // Respond to a two-finger shove with the given distance
    void handleShoveGesture(float _distance);
    
    // Set debug features on or off using a boolean (see debug.h)
    void setDebugFlag(DebugFlags _flag, bool _on);
    
    // Get the boolean state of a debug feature (see debug.h)
    bool getDebugFlag(DebugFlags _flag);

}

