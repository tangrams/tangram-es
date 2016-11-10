#pragma once

#include "data/properties.h"
#include "util/types.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Tangram {

class TileSource;

struct TouchItem {
    std::shared_ptr<Properties> properties;
    float position[2];
    float distance;
};

struct SceneUpdate {
    std::string path;
    std::string value;
};

enum class EaseType : char {
    linear = 0,
    cubic,
    quint,
    sine,
};

class Map {

public:

    // Create an empty map object. To display a map, call either loadScene() or loadSceneAsync().
    Map();
    ~Map();

    // Load the scene at the given absolute file path asynchronously
    void loadSceneAsync(const char* _scenePath, bool _useScenePosition = false,
            std::function<void(void*)> _platformCallback = {}, void *_cbData = nullptr);

    // Load the scene at the given absolute file path synchronously
    void loadScene(const char* _scenePath, bool _useScenePosition = false);

    // Request an update to the scene configuration; the path is a series of yaml keys
    // separated by a '.' and the value is a string of yaml to replace the current value
    // at the given path in the scene
    void queueSceneUpdate(const char* _path, const char* _value);

    // Apply all previously requested scene updates
    void applySceneUpdates();

    // Set an MBTiles SQLite database file for a DataSource in the scene.
    void setMBTiles(const char* _dataSourceName, const char* _mbtilesFilePath);

    // Initialize graphics resources; OpenGL context must be created prior to calling this
    void setupGL();

    // Resize the map view to a new width and height (in pixels)
    void resize(int _newWidth, int _newHeight);

    // Update the map state with the time interval since the last update, returns
    // true when the current view is completely loaded (all tiles are available and
    // no animation in progress)
    bool update(float _dt);

    // Render a new frame of the map view (if needed)
    void render();

    // Gets the viewport height in physical pixels (framebuffer size)
    int getViewportHeight();

    // Gets the viewport width in physical pixels (framebuffer size)
    int getViewportWidth();

    // Set the ratio of hardware pixels to logical pixels (defaults to 1.0)
    void setPixelScale(float _pixelsPerPoint);

    // Gets the pixel scale
    float getPixelScale();

    // Capture a snapshot of the current frame and store it in the allocated _data
    // _data is expected to be of size getViewportHeight() * getViewportWidth()
    // Pixel data is stored starting from the lower left corner of the viewport
    // Each pixel(x, y) would be located at _data[y * getViewportWidth() + x]
    // Each unsigned int corresponds to an RGBA pixel value
    void captureSnapshot(unsigned int* _data);

    // Set the position of the map view in degrees longitude and latitude; if duration
    // (in seconds) is provided, position eases to the set value over the duration;
    // calling either version of the setter overrides all previous calls
    void setPosition(double _lon, double _lat);
    void setPositionEased(double _lon, double _lat, float _duration, EaseType _e = EaseType::quint);

    // Set the values of the arguments to the position of the map view in degrees
    // longitude and latitude
    void getPosition(double& _lon, double& _lat);

    // Set the fractional zoom level of the view; if duration (in seconds) is provided,
    // zoom eases to the set value over the duration; calling either version of the setter
    // overrides all previous calls
    void setZoom(float _z);
    void setZoomEased(float _z, float _duration, EaseType _e = EaseType::quint);

    // Get the fractional zoom level of the view
    float getZoom();

    // Set the counter-clockwise rotation of the view in radians; 0 corresponds to
    // North pointing up; if duration (in seconds) is provided, rotation eases to the
    // the set value over the duration; calling either version of the setter overrides
    // all previous calls
    void setRotation(float _radians);
    void setRotationEased(float _radians, float _duration, EaseType _e = EaseType::quint);

    // Get the counter-clockwise rotation of the view in radians; 0 corresponds to
    // North pointing up
    float getRotation();

    // Set the tilt angle of the view in radians; 0 corresponds to straight down;
    // if duration (in seconds) is provided, tilt eases to the set value over the
    // duration; calling either version of the setter overrides all previous calls
    void setTilt(float _radians);
    void setTiltEased(float _radians, float _duration, EaseType _e = EaseType::quint);

    // Get the tilt angle of the view in radians; 0 corresponds to straight down
    float getTilt();

    // Set the camera type (0 = perspective, 1 = isometric, 2 = flat)
    void setCameraType(int _type);

    // Get the camera type (0 = perspective, 1 = isometric, 2 = flat)
    int getCameraType();

    // Given coordinates in screen space (x right, y down), set the output longitude and
    // latitude to the geographic location corresponding to that point; returns false if
    // no geographic position corresponds to the screen location, otherwise returns true
    bool screenPositionToLngLat(double _x, double _y, double* _lng, double* _lat);

    // Given longitude and latitude coordinates, set the output coordinates to the
    // corresponding point in screen space (x right, y down); returns false if the
    // point is not visible on the screen, otherwise returns true
    bool lngLatToScreenPosition(double _lng, double _lat, double* _x, double* _y);

    // Add a tile source for adding drawable map data, which will be styled
    // according to the scene file using the provided data source name;
    void addTileSource(std::shared_ptr<TileSource> _source);

    // Remove a tile source from the map; returns true if the source was found
    // and removed, otherwise returns false.
    bool removeTileSource(TileSource& _source);

    void clearTileSource(TileSource& _source, bool _data, bool _tiles);

    // Add a marker object to the map and return an ID for it; an ID of 0 indicates an invalid marker;
    // the marker will not be drawn until both styling and geometry are set using the functions below.
    MarkerID markerAdd();

    // Remove a marker object from the map; returns true if the marker ID was found and successfully
    // removed, otherwise returns false.
    bool markerRemove(MarkerID _marker);

    // Set the styling for a marker object; _styling is a string of YAML that specifies a 'draw rule'
    // according to the scene file syntax; returns true if the marker ID was found and successfully
    // updated, otherwise returns false.
    bool markerSetStyling(MarkerID _marker, const char* _styling);

    // Set a bitmap to use as the image for a point marker; _data is a buffer of RGBA pixel data with
    // length of _width * _height; pixels are in row-major order beginning from the bottom-left of the
    // image; returns true if the marker ID was found and successfully updated, otherwise returns false.
    bool markerSetBitmap(MarkerID _marker, int _width, int _height, const unsigned int* _data);

    // Set the geometry of a marker to a point at the given coordinates; markers can have their
    // geometry set multiple times with possibly different geometry types; returns true if the
    // marker ID was found and successfully updated, otherwise returns false.
    bool markerSetPoint(MarkerID _marker, LngLat _lngLat);

    // Set the geometry of a marker to a point at the given coordinates; if the marker was previously
    // set to a point, this eases the position over the given duration in seconds with the given EaseType;
    // returns true if the marker ID was found and successfully updated, otherwise returns false.
    bool markerSetPointEased(MarkerID _marker, LngLat _lngLat, float _duration, EaseType _ease);

    // Set the geometry of a marker to a polyline along the given coordinates; _coordinates is a
    // pointer to a sequence of _count LngLats; markers can have their geometry set multiple times
    // with possibly different geometry types; returns true if the marker ID was found and
    // successfully updated, otherwise returns false.
    bool markerSetPolyline(MarkerID _marker, LngLat* _coordinates, int _count);

    // Set the geometry of a marker to a polygon with the given coordinates; _counts is a pointer
    // to a sequence of _rings integers and _coordinates is a pointer to a sequence of LngLats with
    // a total length equal to the sum of _counts; for each integer n in _counts, a polygon is created
    // by taking the next n LngLats from _coordinates, with winding order and internal polygons
    // behaving according to the GeoJSON specification; markers can have their geometry set multiple
    // times with possibly different geometry types; returns true if the marker ID was found and
    // successfully updated, otherwise returns false.
    bool markerSetPolygon(MarkerID _marker, LngLat* _coordinates, int* _counts, int _rings);

    // Set the visibility of a marker object; returns true if the marker ID was found and successfully
    // updated, otherwise returns false.
    bool markerSetVisible(MarkerID _marker, bool _visible);

    // Set the ordering of point marker object relative to other markers; higher values are drawn 'above';
    // returns true if the marker ID was found and successfully updated, otherwise returns false.
    bool markerSetDrawOrder(MarkerID _marker, int _drawOrder);

    // Remove all marker objects from the map; Any marker IDs previously returned from 'markerAdd'
    // are invalidated after this.
    void markerRemoveAll();

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

    // Set whether the OpenGL state will be cached between subsequent frames; this improves rendering
    // efficiency, but can cause errors if your application code makes OpenGL calls (false by default)
    void useCachedGlState(bool _use);

    void pickFeaturesAt(float _x, float _y, std::function<void(const std::vector<TouchItem>&)> _onReadyCallback);

    // Run this task asynchronously to Tangram's main update loop.
    void runAsyncTask(std::function<void()> _task);

private:

    class Impl;
    std::unique_ptr<Impl> impl;

};

enum DebugFlags {
    freeze_tiles = 0,   // While on, the set of tiles currently being drawn will not update to match the view
    proxy_colors,       // Applies a color change to every other zoom level of tiles to visualize proxy tile behavior
    tile_bounds,        // Draws tile boundaries
    tile_infos,         // Debug tile infos
    labels,             // Debug label bounding boxes
    tangram_infos,      // Various text tangram debug info printed on the screen
    draw_all_labels,    // Draw all labels
    tangram_stats,      // Tangram frame graph stats
    selection_buffer,   // Render selection framebuffer
};

// Set debug features on or off using a boolean (see debug.h)
void setDebugFlag(DebugFlags _flag, bool _on);

// Get the boolean state of a debug feature (see debug.h)
bool getDebugFlag(DebugFlags _flag);

// Toggle the boolean state of a debug feature (see debug.h)
void toggleDebugFlag(DebugFlags _flag);

}

