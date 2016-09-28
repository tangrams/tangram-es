#ifndef _TANGRAM_CORE_H_
#define _TANGRAM_CORE_H_

#include "tangram-core-export.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef void* tangram_map_t;
typedef void* tangram_data_source_t;

typedef void (*tangram_function_t)();
typedef void(*tangram_function_with_arg_t)(void*);

typedef void (*tangram_url_fetch_cancel_t)(const char*url);
typedef void (*tangram_url_fetch_callback_t)(void* context, char* buffer, size_t sz);
typedef bool (*tangram_url_fetch_t)(void* context, const char* url, tangram_url_fetch_callback_t callback);

typedef enum TangramEaseType {
    TangramEaseLinear = 0,
    TangramEaseCubic,
    TangramEaseQuint,
    TangramEaseSine,
} TangramEaseType;

typedef enum TangramDebugFlags {
    TangramDebugFlagFreezeTiles = 0,   // While on, the set of tiles currently being drawn will not update to match the view
    TangramDebugFlagProxyColors,       // Applies a color change to every other zoom level of tiles to visualize proxy tile behavior
    TangramDebugFlagTileBounds,        // Draws tile boundaries
    TangramDebugFlagTileInfos,         // Debug tile infos
    TangramDebugFlagLabels,             // Debug label bounding boxes
    TangramDebugFlagTangramInfos,      // Various text tangram debug info printed on the screen
    TangramDebugFlagDrawAllLabels,         // Draw all labels
    TangramDebugFlagTangramStats,      // Tangram frame graph stats
} TangramDebugFlags;

typedef struct TangramRange {
    int start;
    int length;
} TangramRange;

typedef struct TangramLngLat {
    double longitude;
    double latitude;
#ifdef __cplusplus
    TangramLngLat() :longitude(0.0), latitude(0.0) {}
    TangramLngLat(double _lon, double _lat) : longitude(_lon), latitude(_lat) {}

    TangramLngLat(const TangramLngLat& _other) = default;
    TangramLngLat(TangramLngLat&& _other) = default;
    TangramLngLat& operator=(const TangramLngLat& _other) = default;
    TangramLngLat& operator=(TangramLngLat&& _other) = default;

    bool operator==(const TangramLngLat& _other) {
        return longitude == _other.longitude &&
            latitude == _other.latitude;
    }
#endif
} TangramLngLat;

typedef struct TangramPointerArray {
    void* data;
    size_t size;
} TangramPointerArray;

typedef TangramPointerArray TangramString; // data is char*

typedef struct TangramTouchItem {
    TangramString properties; // JSON string
    float position[2];
    float distance;
} TangramTouchItem;

typedef TangramPointerArray TangramTouchItemArray; // data is TangramTouchItem

// Init, working & finish functions
TANGRAM_CORE_EXPORT void tangramInit(tangram_function_t render);
TANGRAM_CORE_EXPORT void tangramLogMsg(const char* fmt, ...);
TANGRAM_CORE_EXPORT void tangramSetLogFile(FILE *file);

TANGRAM_CORE_EXPORT void tangamSetResourceDir(const char* resourceDirPath);

TANGRAM_CORE_EXPORT void tangamUrlFetcherRunner();
TANGRAM_CORE_EXPORT bool tangamUrlFetcherDefault(void* context, const char* url, tangram_url_fetch_callback_t callback);
TANGRAM_CORE_EXPORT void tangamUrlCancelerDefault(const char*url);
TANGRAM_CORE_EXPORT void tangamUrlFetcherStop();
TANGRAM_CORE_EXPORT void tangramRegisterUrlFetcher(size_t parallelCount, tangram_url_fetch_t fetcher, tangram_url_fetch_cancel_t cancler);

// Map
TANGRAM_CORE_EXPORT tangram_map_t tangramMapCreate();
// Load the scene at the given absolute file path asynchronously
TANGRAM_CORE_EXPORT void tangramLoadSceneAsync(tangram_map_t map, const char* _scenePath, bool _useScenePosition, tangram_function_with_arg_t _platformCallback, void*arg);

// Load the scene at the given absolute file path synchronously
TANGRAM_CORE_EXPORT void tangramLoadScene(tangram_map_t map, const char* _scenePath, bool _useScenePosition);

// Request an update to the scene configuration; the path is a series of yaml keys
// separated by a '.' and the value is a string of yaml to replace the current value
// at the given path in the scene
TANGRAM_CORE_EXPORT void tangramQueueSceneUpdate(tangram_map_t map, const char* _path, const char* _value);

// Apply all previously requested scene updates
TANGRAM_CORE_EXPORT void tangramApplySceneUpdates(tangram_map_t map);

// Initialize graphics resources; OpenGL context must be created prior to calling this
TANGRAM_CORE_EXPORT void tangramSetupGL(tangram_map_t map);

// Resize the map view to a new width and height (in pixels)
TANGRAM_CORE_EXPORT void tangramResize(tangram_map_t map, int _newWidth, int _newHeight);

// Update the map state with the time interval since the last update, returns
// true when the current view is completely loaded (all tiles are available and
// no animation in progress)
TANGRAM_CORE_EXPORT bool tangramUpdate(tangram_map_t map, float _dt);

// Render a new frame of the map view (if needed)
TANGRAM_CORE_EXPORT void tangramRender(tangram_map_t map);

// Gets the viewport height in physical pixels (framebuffer size)
TANGRAM_CORE_EXPORT int tangramGetViewportHeight(tangram_map_t map);

// Gets the viewport width in physical pixels (framebuffer size)
TANGRAM_CORE_EXPORT int tangramGetViewportWidth(tangram_map_t map);

// Set the ratio of hardware pixels to logical pixels (defaults to 1.0)
TANGRAM_CORE_EXPORT void tangramSetPixelScale(tangram_map_t map, float _pixelsPerPoint);

// Gets the pixel scale
TANGRAM_CORE_EXPORT float tangramGetPixelScale(tangram_map_t map);

// Capture a snapshot of the current frame and store it in the allocated _data
// _data is expected to be of size getViewportHeight() * getViewportWidth()
// Pixel data is stored starting from the lower left corner of the viewport
// Each pixel(x, y) would be located at _data[y * getViewportWidth() + x]
// Each unsigned int corresponds to an RGBA pixel value
TANGRAM_CORE_EXPORT void tangramCaptureSnapshot(tangram_map_t map, unsigned int* _data);

// Set the position of the map view in degrees longitude and latitude; if duration
// (in seconds) is provided, position eases to the set value over the duration;
// calling either version of the setter overrides all previous calls
TANGRAM_CORE_EXPORT void tangramSetPosition(tangram_map_t map, double _lon, double _lat);
TANGRAM_CORE_EXPORT void tangramSetPositionEased(tangram_map_t map, double _lon, double _lat, float _duration, TangramEaseType _e);

// Set the values of the arguments to the position of the map view in degrees
// longitude and latitude
TANGRAM_CORE_EXPORT void tangramGetPosition(tangram_map_t map, double* _lon, double* _lat);

// Set the fractional zoom level of the view; if duration (in seconds) is provided,
// zoom eases to the set value over the duration; calling either version of the setter
// overrides all previous calls
TANGRAM_CORE_EXPORT void tangramSetZoom(tangram_map_t map, float _z);
TANGRAM_CORE_EXPORT void tangramSetZoomEased(tangram_map_t map, float _z, float _duration, TangramEaseType _e);

// Get the fractional zoom level of the view
TANGRAM_CORE_EXPORT float tangramGetZoom(tangram_map_t map);

// Set the counter-clockwise rotation of the view in radians; 0 corresponds to
// North pointing up; if duration (in seconds) is provided, rotation eases to the
// the set value over the duration; calling either version of the setter overrides
// all previous calls
TANGRAM_CORE_EXPORT void tangramSetRotation(tangram_map_t map, float _radians);
TANGRAM_CORE_EXPORT void tangramSetRotationEased(tangram_map_t map, float _radians, float _duration, TangramEaseType _e);

// Get the counter-clockwise rotation of the view in radians; 0 corresponds to
// North pointing up
TANGRAM_CORE_EXPORT float tangramGetRotation(tangram_map_t map);

// Set the tilt angle of the view in radians; 0 corresponds to straight down;
// if duration (in seconds) is provided, tilt eases to the set value over the
// duration; calling either version of the setter overrides all previous calls
TANGRAM_CORE_EXPORT void tangramSetTilt(tangram_map_t map, float _radians);
TANGRAM_CORE_EXPORT void tangramSetTiltEased(tangram_map_t map, float _radians, float _duration, TangramEaseType _e);

// Get the tilt angle of the view in radians; 0 corresponds to straight down
TANGRAM_CORE_EXPORT float tangramGetTilt(tangram_map_t map);

// Set the camera type (0 = perspective, 1 = isometric, 2 = flat)
TANGRAM_CORE_EXPORT void tangramSetCameraType(tangram_map_t map, int _type);

// Get the camera type (0 = perspective, 1 = isometric, 2 = flat)
TANGRAM_CORE_EXPORT int tangramGetCameraType(tangram_map_t map);

// Given coordinates in screen space (x right, y down), set the output longitude and
// latitude to the geographic location corresponding to that point; returns false if
// no geographic position corresponds to the screen location, otherwise returns true
TANGRAM_CORE_EXPORT bool tangramScreenPositionToLngLat(tangram_map_t map, double _x, double _y, double* _lng, double* _lat);

// Given longitude and latitude coordinates, set the output coordinates to the
// corresponding point in screen space (x right, y down); returns false if the
// point is not visible on the screen, otherwise returns true
TANGRAM_CORE_EXPORT bool tangramLngLatToScreenPosition(tangram_map_t map, double _lng, double _lat, double* _x, double* _y);

// Add a data source for adding drawable map data, which will be styled
// according to the scene file using the provided data source name;
TANGRAM_CORE_EXPORT void tangramAddDataSource(tangram_map_t map, tangram_data_source_t _source);

// Remove a data source from the map; returns true if the source was found
// and removed, otherwise returns false.
TANGRAM_CORE_EXPORT bool tangramRemoveDataSource(tangram_map_t map, tangram_data_source_t _source);

TANGRAM_CORE_EXPORT void tangramClearDataSource(tangram_map_t map, tangram_data_source_t _source, bool _data, bool _tiles);

// Respond to a tap at the given screen coordinates (x right, y down)
TANGRAM_CORE_EXPORT void tangramHandleTapGesture(tangram_map_t map, float _posX, float _posY);

// Respond to a double tap at the given screen coordinates (x right, y down)
TANGRAM_CORE_EXPORT void tangramHandleDoubleTapGesture(tangram_map_t map, float _posX, float _posY);

// Respond to a drag with the given displacement in screen coordinates (x right, y down)
TANGRAM_CORE_EXPORT void tangramHandlePanGesture(tangram_map_t map, float _startX, float _startY, float _endX, float _endY);

// Respond to a fling from the given position with the given velocity in screen coordinates
TANGRAM_CORE_EXPORT void tangramHandleFlingGesture(tangram_map_t map, float _posX, float _posY, float _velocityX, float _velocityY);

// Respond to a pinch at the given position in screen coordinates with the given
// incremental scale
TANGRAM_CORE_EXPORT void tangramHandlePinchGesture(tangram_map_t map, float _posX, float _posY, float _scale, float _velocity);

// Respond to a rotation gesture with the given incremental rotation in radians
TANGRAM_CORE_EXPORT void tangramHandleRotateGesture(tangram_map_t map, float _posX, float _posY, float _rotation);

// Respond to a two-finger shove with the given distance in screen coordinates
TANGRAM_CORE_EXPORT void tangramHandleShoveGesture(tangram_map_t map, float _distance);

// Set whether the OpenGL state will be cached between subsequent frames; this improves rendering
// efficiency, but can cause errors if your application code makes OpenGL calls (false by default)
TANGRAM_CORE_EXPORT void tangramUseCachedGlState(tangram_map_t map, bool _use);

TANGRAM_CORE_EXPORT void tangramPickFeaturesAt(tangram_map_t map, float _x, float _y, void(*)(TangramTouchItemArray));

// Run this task asynchronously to Tangram's main update loop.
TANGRAM_CORE_EXPORT void tangramRunAsyncTask(tangram_map_t map, tangram_function_t _task);

TANGRAM_CORE_EXPORT void tangramMapDestroy(tangram_map_t map_handle);

// Data source
TANGRAM_CORE_EXPORT tangram_data_source_t tangramDataSourceCreate(const char* name, const char* url, int32_t maxzoom);
/*
If json_length == SIZE_MAX, then means the json is null terminated
*/
TANGRAM_CORE_EXPORT void tangramDataSourceAddGeoJSON(tangram_data_source_t source, const char*json, size_t json_length);

TANGRAM_CORE_EXPORT void tangramDataSourceRemoveFeature(tangram_data_source_t source, uint32_t featureId);

TANGRAM_CORE_EXPORT void tangramDataSourceDestroy(tangram_data_source_t source);

// Continuous Rendering
TANGRAM_CORE_EXPORT void tangramSetContinuousRendering(bool _isContinuous);
TANGRAM_CORE_EXPORT bool tangramIsContinuousRendering();

// Debug flags

// Set debug features on or off using a boolean (see debug.h)
TANGRAM_CORE_EXPORT void tangramSetDebugFlag(TangramDebugFlags _flag, bool _on);

// Get the boolean state of a debug feature (see debug.h)
TANGRAM_CORE_EXPORT bool tangramGetDebugFlag(TangramDebugFlags _flag);

// Toggle the boolean state of a debug feature (see debug.h)
TANGRAM_CORE_EXPORT void tangramToggleDebugFlag(TangramDebugFlags _flag);

#ifdef __cplusplus
}
#endif

#endif // _TANGRAM_CORE_H_