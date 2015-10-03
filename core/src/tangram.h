#pragma once

#include "debug.h"
#include <vector>
#include <string>

#include <memory>
#include "data/properties.h"

/* Tangram API
 *
 * Primary interface for controlling and managing the lifecycle of a Tangram map surface
 */

namespace Tangram {

// Create resources and initialize the map view using the scene file at the given resource path
void initialize(const char* _scenePath);

// Initialize graphics resources; OpenGL context must be created prior to calling this
void setupGL();

// Resize the map view to a new width and height (in pixels)
void resize(int _newWidth, int _newHeight);

// Update the map state with the time interval since the last update
void update(float _dt);

// Render a new frame of the map view (if needed)
void render();

// Set the position of the map view in degrees longitude and latitude
void setPosition(double _lon, double _lat);

// Set the values of the arguments to the position of the map view in degrees longitude and latitude
void getPosition(double& _lon, double& _lat);

// Set the fractional zoom level of the view
void setZoom(float _z);

// Get the fractional zoom level of the view
float getZoom();

// Set the counter-clockwise rotation of the view in radians; 0 corresponds to North pointing up
void setRotation(float _radians);

// Get the counter-clockwise rotation of the view in radians; 0 corresponds to North pointing up
float getRotation();

// Set the tilt angle of the view in radians; 0 corresponds to straight down
void setTilt(float _radians);

// Get the tilt angle of the view in radians; 0 corresponds to straight down
float getTilt();

// Transform coordinates in screen space (x right, y down) into their longitude and latitude in the map view
void screenToWorldCoordinates(double& _x, double& _y);

// Set the ratio of hardware pixels to logical pixels (defaults to 1.0)
void setPixelScale(float _pixelsPerPoint);

// Create a data source for adding drawable map data, which will be styled according to the scene file using the provided data source name;
// Returns an integer ID for use with the addSource* and clearSource* functions
int addDataSource(const char* _name);

// Removes all data from the source with the provided ID
void clearSourceData(int _sourceId);

// Add a point geometry to the source with the provided ID; coordinates are longitude and latitude in a two-element double-precision array
void addSourcePoint(int _sourceId, double* _coordinates);

// Add a line geometry to the source with the provided ID; coordinates are a sequence of longitude and latitude pairs,
// lineLength is the number of points in the line
void addSourceLine(int _sourceId, double* _coordinates, int _lineLength);

// Add a polygon geometry to the source with the provided ID; coordinates are a sequence of longitude and latitude pairs,
// ringLengths is a sequence of successive counts of coordinates that represent rings in the polygon (the sum of ringLengths is
// the total number of coordinate pairs) and rings is the number of rings in the polygon (and the number of ringLengths)
void addSourcePoly(int _sourceId, double* _coordinates, int* _ringLengths, int rings);

// Add geometry from a GeoJSON string to the source with the provided ID
void addSourceGeoJSON(int _sourceID, const char* _data);

// Respond to a tap at the given screen coordinates (x right, y down)
void handleTapGesture(float _posX, float _posY);

// Respond to a double tap at the given screen coordinates (x right, y down)
void handleDoubleTapGesture(float _posX, float _posY);

// Respond to a drag with the given displacement in screen coordinates (x right, y down)
void handlePanGesture(float _startX, float _startY, float _endX, float _endY);

// Respond to a pinch at the given position in screen coordinates with the given incremental scale
void handlePinchGesture(float _posX, float _posY, float _scale, float _velocity);

// Respond to a rotation gesture with the given incremental rotation in radians
void handleRotateGesture(float _posX, float _posY, float _rotation);

// Respond to a two-finger shove with the given distance
void handleShoveGesture(float _distance);

// Set debug features on or off using a boolean (see debug.h)
void setDebugFlag(DebugFlags _flag, bool _on);

// Get the boolean state of a debug feature (see debug.h)
bool getDebugFlag(DebugFlags _flag);

// Toggle the boolean state of a debug feature (see debug.h)
void toggleDebugFlag(DebugFlags _flag);

void loadScene(const char* _scenePath);

const std::vector<std::shared_ptr<Properties>>& pickFeaturesAt(float _x, float _y);

}

