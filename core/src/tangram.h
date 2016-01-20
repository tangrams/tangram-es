#pragma once

#include "debug.h"
#include "data/properties.h"
#include "util/ease.h"
#include <memory>
#include <vector>
#include <string>

/* Tangram API
 *
 * Primary interface for controlling and managing the lifecycle of a Tangram
 * map surface
 */

namespace Tangram {

class DataSource;

// Create resources and initialize the map view using the scene file at the
// given resource path
void initialize(const char* _scenePath, float _pixelScale = 1);

void shutdown();

// Initialize graphics resources; OpenGL context must be created prior to calling this
void setupGL();

// Resize the map view to a new width and height (in pixels)
void resize(int _newWidth, int _newHeight);

// Update the map state with the time interval since the last update
void update(float _dt);

// Render a new frame of the map view (if needed)
void render();

// Set the position of the map view in degrees longitude and latitude; if duration
// (in seconds) is provided, position eases to the set value over the duration;
// calling either version of the setter overrides all previous calls
void setPosition(double _lon, double _lat);
void setPosition(double _lon, double _lat, float _duration, EaseType _e = EaseType::quint);

// Set the values of the arguments to the position of the map view in degrees
// longitude and latitude
void getPosition(double& _lon, double& _lat);

// Set the fractional zoom level of the view; if duration (in seconds) is provided,
// zoom eases to the set value over the duration; calling either version of the setter
// overrides all previous calls
void setZoom(float _z);
void setZoom(float _z, float _duration, EaseType _e = EaseType::quint);

// Get the fractional zoom level of the view
float getZoom();

// Set the counter-clockwise rotation of the view in radians; 0 corresponds to
// North pointing up; if duration (in seconds) is provided, rotation eases to the
// the set value over the duration; calling either version of the setter overrides
// all previous calls
void setRotation(float _radians);
void setRotation(float _radians, float _duration, EaseType _e = EaseType::quint);

// Get the counter-clockwise rotation of the view in radians; 0 corresponds to
// North pointing up
float getRotation();

// Set the tilt angle of the view in radians; 0 corresponds to straight down;
// if duration (in seconds) is provided, tilt eases to the set value over the
// duration; calling either version of the setter overrides all previous calls
void setTilt(float _radians);
void setTilt(float _radians, float _duration, EaseType _e = EaseType::quint);

// Get the tilt angle of the view in radians; 0 corresponds to straight down
float getTilt();

// Transform coordinates in screen space (x right, y down) into their longitude
// and latitude in the map view
void screenToWorldCoordinates(double& _x, double& _y);

// Set the ratio of hardware pixels to logical pixels (defaults to 1.0)

// Add a data source for adding drawable map data, which will be styled
// according to the scene file using the provided data source name;
void addDataSource(std::shared_ptr<DataSource> _source);

int removeDataSource(DataSource& _source);

void clearDataSource(DataSource& _source, bool _data, bool _tiles);

// Respond to a tap at the given screen coordinates (x right, y down)
void handleTapGesture(float _posX, float _posY);

// Respond to a double tap at the given screen coordinates (x right, y down)
void handleDoubleTapGesture(float _posX, float _posY);

// Respond to a drag with the given displacement in screen coordinates (x right, y down)
void handlePanGesture(float _startX, float _startY, float _endX, float _endY);

// Respond to a fling from the given position with the given velocity in screen coordinates
void handleFlingGesture(float _posX, float _posY, float _velocityX, float _velocityY);

// Respond to a pinch at the given position in screen coordinates with the given
// incremental scale
void handlePinchGesture(float _posX, float _posY, float _scale, float _velocity);

// Respond to a rotation gesture with the given incremental rotation in radians
void handleRotateGesture(float _posX, float _posY, float _rotation);

// Respond to a two-finger shove with the given distance in screen coordinates
void handleShoveGesture(float _distance);

// Set debug features on or off using a boolean (see debug.h)
void setDebugFlag(DebugFlags _flag, bool _on);

// Get the boolean state of a debug feature (see debug.h)
bool getDebugFlag(DebugFlags _flag);

// Toggle the boolean state of a debug feature (see debug.h)
void toggleDebugFlag(DebugFlags _flag);

void loadScene(const char* _scenePath, bool _setPositionFromScene = false);

struct TouchItem {
    std::shared_ptr<Properties> properties;
    float distance;
};

const std::vector<TouchItem>& pickFeaturesAt(float _x, float _y);

}

