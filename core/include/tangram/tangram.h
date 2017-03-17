#pragma once

#include "data/properties.h"
#include "util/types.h"

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Tangram {

class Platform;
class TileSource;

enum LabelType {
    icon,
    text,
};

struct FeaturePickResult {
    FeaturePickResult(std::shared_ptr<Properties> _properties,
                      std::array<float, 2> _position)
        : properties(_properties), position(_position) {}

    std::shared_ptr<Properties> properties;
    std::array<float, 2> position;
};

// Returns a pointer to the selected feature pick result or null, only valid on the callback scope
using FeaturePickCallback = std::function<void(const FeaturePickResult*)>;

struct LabelPickResult {
    LabelPickResult(LabelType _type, LngLat _coordinates, FeaturePickResult _touchItem)
        : type(_type),
          coordinates(_coordinates),
          touchItem(_touchItem) {}

    LabelType type;
    LngLat coordinates;
    FeaturePickResult touchItem;
};

// Returns a pointer to the selected label pick result or null, only valid on the callback scope
using LabelPickCallback = std::function<void(const LabelPickResult*)>;

struct MarkerPickResult {
    MarkerPickResult(MarkerID _id, LngLat _coordinates, std::array<float, 2> _position)
        : id(_id), coordinates(_coordinates), position(_position) {}

    MarkerID id;
    LngLat coordinates;
    std::array<float, 2> position;
};

// Returns a pointer to the selected marker pick result or null, only valid on the callback scope
using MarkerPickCallback = std::function<void(const MarkerPickResult*)>;

struct SceneUpdate {
    std::string path;
    std::string value;
    SceneUpdate(std::string p, std::string v) : path(p), value(v) {}
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
    Map(std::shared_ptr<Platform> _platform);
    ~Map();

    // Load the scene at the given absolute file path asynchronously
    void loadSceneAsync(const char* _scenePath, bool _useScenePosition = false,
                        std::function<void(void*)> _platformCallback = {}, void *_cbData = nullptr,
                        const std::vector<SceneUpdate>& sceneUpdates = {});

    // Load the scene at the given absolute file path synchronously
    void loadScene(const char* _scenePath, bool _useScenePosition = false,
                   const std::vector<SceneUpdate>& sceneUpdates = {});

    // Request an update to the scene configuration; the path is a series of yaml keys
    // separated by a '.' and the value is a string of yaml to replace the current value
    // at the given path in the scene
    void queueSceneUpdate(const char* _path, const char* _value);
    void queueSceneUpdate(const std::vector<SceneUpdate>& sceneUpdates);

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

    // Set the ratio of hardware pixels to logical pixels (defaults to 1.0);
    // this operation can be slow, so only perform this when necessary.
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
    bool markerSetStylingFromString(MarkerID _marker, const char* _styling);

    // Set an explicit draw group for a marker object; _path is a '.' delimited path to a draw rule
    // in the current scene. The Marker will be styled using the draw rule specified at this path;
    // returns true if the marker ID was found and successfully updated, otherwise returns false.
    bool markerSetStylingFromPath(MarkerID _marker, const char* _path);

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

    // Set the radius in logical pixels to use when picking features on the map (default is 0.5).
    void setPickRadius(float _radius);

    // Create a query to select a feature marked as 'interactive'. The query runs on the next frame.
    // Calls _onFeaturePickCallback once the query has completed, and returns the FeaturePickResult
    // with its associated properties or null if no feature was found.
    void pickFeatureAt(float _x, float _y, FeaturePickCallback _onFeaturePickCallback);

    // Create a query to select a label created for a feature marked as 'interactive'. The query runs
    // on the next frame.
    // Calls _onLabelPickCallback once the query has completed, and returns the LabelPickResult
    // with its associated properties or null if no label was found.
    void pickLabelAt(float _x, float _y, LabelPickCallback _onLabelPickCallback);

    // Create a query to select a marker that is 'interactive'. The query runs on the next frame.
    // Calls _onLMarkerPickCallback once the query has completed, and returns the MarkerPickResult
    // with its associated properties or null if no marker was found.
    void pickMarkerAt(float _x, float _y, MarkerPickCallback _onMarkerPickCallback);

    // Run this task asynchronously to Tangram's main update loop.
    void runAsyncTask(std::function<void()> _task);

    std::shared_ptr<Platform>& getPlatform();

private:

    class Impl;
    std::unique_ptr<Impl> impl;

protected:

    std::shared_ptr<Platform> platform;

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
