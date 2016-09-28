#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <functional>
#include <string>
#include <list>
#include <deque>
#include <unordered_set>
#include <regex>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "tangram-core.h"
#include "tangram.h"
#include "platform.h"

#include "urlGet.h"
#include "data/clientGeoJsonSource.h"

namespace {
static bool s_isContinuousRendering = false;

tangram_function_t requesetRenderFunction = nullptr;
tangram_url_fetch_t tangram_url_fetcher = nullptr;
tangram_url_fetch_cancel_t tangram_url_fetch_canceler= nullptr;
std::string resourceDirPathSaved = "";
FILE *logFile = nullptr;
std::atomic_int tangramFetcherIsFinished;
}

inline FILE* getLogFile() {
    if (logFile) {
        return logFile;
    }
    return stderr;
}

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(getLogFile(), fmt, args);
    va_end(args);
}

void requestRender() {
    if (requesetRenderFunction) {
        requesetRenderFunction();
    }
}

void setContinuousRendering(bool _isContinuous) {
    s_isContinuousRendering = _isContinuous;
}

bool isContinuousRendering() {
    return s_isContinuousRendering;
}

void initGLExtensions() {
}

std::string stringFromFile(const char* _path) {
    size_t length = 0;
    unsigned char* bytes = bytesFromFile(_path, length);

    std::string out(reinterpret_cast<char*>(bytes), length);
    free(bytes);

    return out;
}

unsigned char* bytesFromFile(const char* _path, size_t& _size) {
    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);
    if (!resource.is_open() && !resourceDirPathSaved.empty()) {
        resource.open(resourceDirPathSaved + '/' + _path, std::ifstream::ate | std::ifstream::binary);
    }
    if (!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        _size = 0;
        return nullptr;
    }

    _size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*)malloc(sizeof(char) * (_size));

    resource.read(cdata, _size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);
}

// No system fonts implementation (yet!)
std::string systemFontPath(const std::string& _name, const std::string& _weight,
    const std::string& _face) {
    return "";
}

// No system fonts fallback implementation (yet!)
std::string systemFontFallbackPath(int _importance, int _weightHint) {
    return "";
}

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {
    if (tangram_url_fetcher) {
        auto context = new UrlCallback(_callback);
        return tangram_url_fetcher(context, _url.c_str(), [](void* context, char* buffer, size_t sz){
            UrlCallback callback = *(UrlCallback*)context;
            delete (UrlCallback*)context;
            std::vector<char> vec(buffer, buffer + sz);
            callback(std::move(vec));
        });
    } else {
        return false;
    }
}

void cancelUrlRequest(const std::string& _url) {
    if (tangram_url_fetch_canceler) {
        tangram_url_fetch_canceler(_url.c_str());
    }
}

void setCurrentThreadPriority(int priority) {
}

void readFile(const std::string &filePath, std::vector<char> &stream) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (file.fail()) {
        return;
    }
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    stream.resize(size);
    if (!file.read(stream.data(), size))
    {
        stream.resize(0);
    }
    file.close();
}

struct fetch_context {
    void* context;
    std::string url;
    tangram_url_fetch_callback_t callback;
};

class Semaphore {
public:
    Semaphore(int count_ = 0)
        : count(count_) {}

    inline void notify()
    {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        cv.notify_one();
    }

    inline void wait()
    {
        std::unique_lock<std::mutex> lock(mtx);

        while (count == 0) {
            cv.wait(lock);
        }
        count--;
    }

private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
};
std::deque<fetch_context> fetch_contextes;
std::unordered_set<std::string> pending_fetches;
std::unordered_set<std::string> fetch_canceles;
std::mutex fetch_contextes_mutex;
Semaphore fetch_semaphore(0);

void fetch_contexts_add(const fetch_context& new_context) {
    fetch_contextes_mutex.lock();
    if (pending_fetches.find(new_context.url) == pending_fetches.end()) {
        pending_fetches.insert(new_context.url);
        fetch_contextes.push_back(new_context);
    }
    fetch_contextes_mutex.unlock();
    fetch_semaphore.notify();
}

bool fetch_contexts_get(fetch_context &context, bool &canceled) {
    bool hasContext;
    fetch_contextes_mutex.lock();
    hasContext = !fetch_contextes.empty();
    if (hasContext) {
        context = fetch_contextes.front();
        canceled = fetch_canceles.find(context.url) != fetch_canceles.end();
        if (canceled) {
            fetch_canceles.erase(context.url);
        }
        pending_fetches.erase(context.url);
        fetch_contextes.pop_front();
    }
    fetch_contextes_mutex.unlock();
    return hasContext;
}

void fetch_contexts_cancel(const std::string &url) {
    fetch_contextes_mutex.lock();
    if (pending_fetches.find(url) != pending_fetches.end()) {
        fetch_canceles.insert(url);
    }
    fetch_contextes_mutex.unlock();
}

extern "C" {
TANGRAM_CORE_EXPORT void tangramInit(tangram_function_t render) {
    requesetRenderFunction = render;
}

// TODO: merge with LogMsg
TANGRAM_CORE_EXPORT void tangramLogMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(getLogFile(), fmt, args);
    va_end(args);
}

TANGRAM_CORE_EXPORT void tangramSetLogFile(FILE *file) {
    logFile = file;
}

TANGRAM_CORE_EXPORT void tangamSetResourceDir(const char* resourceDirPath) {
    resourceDirPathSaved = resourceDirPath;
}

TANGRAM_CORE_EXPORT void tangamUrlFetcherRunner() {
    std::vector<char> stream;
    fetch_context context;
    bool canceled;
    tangramFetcherIsFinished = 0;
    while (!tangramFetcherIsFinished.load()) {
        bool hasContext = fetch_contexts_get(context, canceled);
        if (hasContext) {
            stream.resize(0);
            if (!canceled) {
                const std::string &url = context.url;
                if (url.find("file:///") == 0) {
                    readFile(url.substr(strlen("file:///")), stream);
                }
                else {
                    if (!urlGet(url, stream)) {
                        stream.resize(0);
                    }
                }
            }
            else {
                // Canceled, then callback with nothing
            }
            if (stream.size() > 0) {
                context.callback(context.context, stream.data(), stream.size());
            }
            continue;
        }
        fetch_semaphore.wait();
    }
}

TANGRAM_CORE_EXPORT bool tangamUrlFetcherDefault(void* context, const char* url, tangram_url_fetch_callback_t callback) {
    fetch_context new_context = {
        context,
        url,
        callback
    };
    fetch_contexts_add(new_context);
    return true;
}

TANGRAM_CORE_EXPORT void tangamUrlCancelerDefault(const char*url) {
    fetch_contexts_cancel(url);
}

TANGRAM_CORE_EXPORT void tangamUrlFetcherStop() {
    tangramFetcherIsFinished.store(1);
}

TANGRAM_CORE_EXPORT void tangramRegisterUrlFetcher(size_t urlWorkerCount, tangram_url_fetch_t fetcher, tangram_url_fetch_cancel_t canceler) {
    tangram_url_fetcher = fetcher;
    tangram_url_fetch_canceler = canceler;
}

TANGRAM_CORE_EXPORT tangram_map_t tangramMapCreate() {
    return new Tangram::Map();
}

// Load the scene at the given absolute file path asynchronously
TANGRAM_CORE_EXPORT void tangramLoadSceneAsync(tangram_map_t map, const char* _scenePath, bool _useScenePosition, tangram_function_with_arg_t _platformCallback, void*arg) {
    if (_platformCallback) {
        return ((Tangram::Map*)map)->loadSceneAsync(_scenePath, _useScenePosition, _platformCallback, arg);
    } else {
        return ((Tangram::Map*)map)->loadSceneAsync(_scenePath, _useScenePosition);
    }
}

// Load the scene at the given absolute file path synchronously
TANGRAM_CORE_EXPORT void tangramLoadScene(tangram_map_t map, const char* _scenePath, bool _useScenePosition) {
    return ((Tangram::Map*)map)->loadScene(_scenePath, _useScenePosition);
}

// Request an update to the scene configuration; the path is a series of yaml keys
// separated by a '.' and the value is a string of yaml to replace the current value
// at the given path in the scene
TANGRAM_CORE_EXPORT void tangramQueueSceneUpdate(tangram_map_t map, const char* _path, const char* _value) {
    return ((Tangram::Map*)map)->queueSceneUpdate(_path, _value);
}

// Apply all previously requested scene updates
TANGRAM_CORE_EXPORT void tangramApplySceneUpdates(tangram_map_t map) {
    return ((Tangram::Map*)map)->applySceneUpdates();
}

// Initialize graphics resources; OpenGL context must be created prior to calling this
TANGRAM_CORE_EXPORT void tangramSetupGL(tangram_map_t map) {
    return ((Tangram::Map*)map)->setupGL();
}

// Resize the map view to a new width and height (in pixels)
TANGRAM_CORE_EXPORT void tangramResize(tangram_map_t map, int _newWidth, int _newHeight) {
    return ((Tangram::Map*)map)->resize(_newWidth, _newHeight);
}

// Update the map state with the time interval since the last update, returns
// true when the current view is completely loaded (all tiles are available and
// no animation in progress)
TANGRAM_CORE_EXPORT bool tangramUpdate(tangram_map_t map, float _dt) {
    return ((Tangram::Map*)map)->update(_dt);
}

// Render a new frame of the map view (if needed)
TANGRAM_CORE_EXPORT void tangramRender(tangram_map_t map) {
    return ((Tangram::Map*)map)->render();
}

// Gets the viewport height in physical pixels (framebuffer size)
TANGRAM_CORE_EXPORT int tangramGetViewportHeight(tangram_map_t map) {
    return ((Tangram::Map*)map)->getViewportHeight();
}

// Gets the viewport width in physical pixels (framebuffer size)
TANGRAM_CORE_EXPORT int tangramGetViewportWidth(tangram_map_t map) {
    return ((Tangram::Map*)map)->getViewportWidth();
}

// Set the ratio of hardware pixels to logical pixels (defaults to 1.0)
TANGRAM_CORE_EXPORT void tangramSetPixelScale(tangram_map_t map, float _pixelsPerPoint) {
    return ((Tangram::Map*)map)->setPixelScale(_pixelsPerPoint);
}

// Gets the pixel scale
TANGRAM_CORE_EXPORT float tangramGetPixelScale(tangram_map_t map) {
    return ((Tangram::Map*)map)->getPixelScale();
}

// Capture a snapshot of the current frame and store it in the allocated _data
// _data is expected to be of size getViewportHeight() * getViewportWidth()
// Pixel data is stored starting from the lower left corner of the viewport
// Each pixel(x, y) would be located at _data[y * getViewportWidth() + x]
// Each unsigned int corresponds to an RGBA pixel value
TANGRAM_CORE_EXPORT void tangramCaptureSnapshot(tangram_map_t map, unsigned int* _data) {
    return ((Tangram::Map*)map)->captureSnapshot(_data);
}

// Set the position of the map view in degrees longitude and latitude; if duration
// (in seconds) is provided, position eases to the set value over the duration;
// calling either version of the setter overrides all previous calls
TANGRAM_CORE_EXPORT void tangramSetPosition(tangram_map_t map, double _lon, double _lat) {
    return ((Tangram::Map*)map)->setPosition(_lon, _lat);
}
TANGRAM_CORE_EXPORT void tangramSetPositionEased(tangram_map_t map, double _lon, double _lat, float _duration, TangramEaseType _e) {
    return ((Tangram::Map*)map)->setPositionEased(_lon, _lat, _duration, (Tangram::EaseType)_e);
}

// Set the values of the arguments to the position of the map view in degrees
// longitude and latitude
TANGRAM_CORE_EXPORT void tangramGetPosition(tangram_map_t map, double* _lon, double* _lat) {
    return ((Tangram::Map*)map)->getPosition(*_lon, *_lat);
}

// Set the fractional zoom level of the view; if duration (in seconds) is provided,
// zoom eases to the set value over the duration; calling either version of the setter
// overrides all previous calls
TANGRAM_CORE_EXPORT void tangramSetZoom(tangram_map_t map, float _z) {
    return ((Tangram::Map*)map)->setZoom(_z);
}
TANGRAM_CORE_EXPORT void tangramSetZoomEased(tangram_map_t map, float _z, float _duration, TangramEaseType _e) {
    return ((Tangram::Map*)map)->setZoomEased(_z, _duration, (Tangram::EaseType)_e);
}

// Get the fractional zoom level of the view
TANGRAM_CORE_EXPORT float tangramGetZoom(tangram_map_t map) {
    return ((Tangram::Map*)map)->getZoom();
}

// Set the counter-clockwise rotation of the view in radians; 0 corresponds to
// North pointing up; if duration (in seconds) is provided, rotation eases to the
// the set value over the duration; calling either version of the setter overrides
// all previous calls
TANGRAM_CORE_EXPORT void tangramSetRotation(tangram_map_t map, float _radians) {
    return ((Tangram::Map*)map)->setRotation(_radians);
}
TANGRAM_CORE_EXPORT void tangramSetRotationEased(tangram_map_t map, float _radians, float _duration, TangramEaseType _e) {
    return ((Tangram::Map*)map)->setRotationEased(_radians, _duration, (Tangram::EaseType)_e);
}

// Get the counter-clockwise rotation of the view in radians; 0 corresponds to
// North pointing up
TANGRAM_CORE_EXPORT float tangramGetRotation(tangram_map_t map) {
    return ((Tangram::Map*)map)->getRotation();
}

// Set the tilt angle of the view in radians; 0 corresponds to straight down;
// if duration (in seconds) is provided, tilt eases to the set value over the
// duration; calling either version of the setter overrides all previous calls
TANGRAM_CORE_EXPORT void tangramSetTilt(tangram_map_t map, float _radians) {
    return ((Tangram::Map*)map)->setTilt(_radians);
}
TANGRAM_CORE_EXPORT void tangramSetTiltEased(tangram_map_t map, float _radians, float _duration, TangramEaseType _e) {
    return ((Tangram::Map*)map)->setTiltEased(_radians, _duration, (Tangram::EaseType)_e);
}

// Get the tilt angle of the view in radians; 0 corresponds to straight down
TANGRAM_CORE_EXPORT float tangramGetTilt(tangram_map_t map) {
    return ((Tangram::Map*)map)->getTilt();
}

// Set the camera type (0 = perspective, 1 = isometric, 2 = flat)
TANGRAM_CORE_EXPORT void tangramSetCameraType(tangram_map_t map, int _type) {
    return ((Tangram::Map*)map)->setCameraType(_type);
}

// Get the camera type (0 = perspective, 1 = isometric, 2 = flat)
TANGRAM_CORE_EXPORT int tangramGetCameraType(tangram_map_t map) {
    return ((Tangram::Map*)map)->getCameraType();
}

// Given coordinates in screen space (x right, y down), set the output longitude and
// latitude to the geographic location corresponding to that point; returns false if
// no geographic position corresponds to the screen location, otherwise returns true
TANGRAM_CORE_EXPORT bool tangramScreenPositionToLngLat(tangram_map_t map, double _x, double _y, double* _lng, double* _lat) {
    return ((Tangram::Map*)map)->screenPositionToLngLat(_x, _y, _lng, _lat);
}

// Given longitude and latitude coordinates, set the output coordinates to the
// corresponding point in screen space (x right, y down); returns false if the
// point is not visible on the screen, otherwise returns true
TANGRAM_CORE_EXPORT bool tangramLngLatToScreenPosition(tangram_map_t map, double _lng, double _lat, double* _x, double* _y) {
    return ((Tangram::Map*)map)->lngLatToScreenPosition(_lng, _lat, _x, _y);
}

// Add a data source for adding drawable map data, which will be styled
// according to the scene file using the provided data source name;
TANGRAM_CORE_EXPORT void tangramAddDataSource(tangram_map_t map, tangram_data_source_t _source) {
    std::shared_ptr<Tangram::DataSource> source;
    source.reset((Tangram::DataSource*)_source);
    return ((Tangram::Map*)map)->addDataSource(source);
}

// Remove a data source from the map; returns true if the source was found
// and removed, otherwise returns false.
TANGRAM_CORE_EXPORT bool tangramRemoveDataSource(tangram_map_t map, tangram_data_source_t _source) {
    return ((Tangram::Map*)map)->removeDataSource(*(Tangram::DataSource*)_source);
}

TANGRAM_CORE_EXPORT void tangramClearDataSource(tangram_map_t map, tangram_data_source_t _source, bool _data, bool _tiles) {
    return ((Tangram::Map*)map)->clearDataSource(*(Tangram::DataSource*)_source, _data, _tiles);
}

// Respond to a tap at the given screen coordinates (x right, y down)
TANGRAM_CORE_EXPORT void tangramHandleTapGesture(tangram_map_t map, float _posX, float _posY) {
    return ((Tangram::Map*)map)->handleTapGesture(_posX, _posY);
}

// Respond to a double tap at the given screen coordinates (x right, y down)
TANGRAM_CORE_EXPORT void tangramHandleDoubleTapGesture(tangram_map_t map, float _posX, float _posY) {
    return ((Tangram::Map*)map)->handleDoubleTapGesture(_posX, _posY);
}

// Respond to a drag with the given displacement in screen coordinates (x right, y down)
TANGRAM_CORE_EXPORT void tangramHandlePanGesture(tangram_map_t map, float _startX, float _startY, float _endX, float _endY) {
    return ((Tangram::Map*)map)->handlePanGesture(_startX, _startY, _endX, _endY);
}

// Respond to a fling from the given position with the given velocity in screen coordinates
TANGRAM_CORE_EXPORT void tangramHandleFlingGesture(tangram_map_t map, float _posX, float _posY, float _velocityX, float _velocityY) {
    return ((Tangram::Map*)map)->handleFlingGesture(_posX, _posY, _velocityX, _velocityY);
}

// Respond to a pinch at the given position in screen coordinates with the given
// incremental scale
TANGRAM_CORE_EXPORT void tangramHandlePinchGesture(tangram_map_t map, float _posX, float _posY, float _scale, float _velocity) {
    return ((Tangram::Map*)map)->handlePinchGesture(_posX, _posY, _scale, _velocity);
}

// Respond to a rotation gesture with the given incremental rotation in radians
TANGRAM_CORE_EXPORT void tangramHandleRotateGesture(tangram_map_t map, float _posX, float _posY, float _rotation) {
    return ((Tangram::Map*)map)->handleRotateGesture(_posX, _posY, _rotation);
}

// Respond to a two-finger shove with the given distance in screen coordinates
TANGRAM_CORE_EXPORT void tangramHandleShoveGesture(tangram_map_t map, float _distance) {
    return ((Tangram::Map*)map)->handleShoveGesture(_distance);
}

// Set whether the OpenGL state will be cached between subsequent frames; this improves rendering
// efficiency, but can cause errors if your application code makes OpenGL calls (false by default)
TANGRAM_CORE_EXPORT void tangramUseCachedGlState(tangram_map_t map, bool _use) {
    return ((Tangram::Map*)map)->useCachedGlState(_use);
}

TANGRAM_CORE_EXPORT void tangramPickFeaturesAt(tangram_map_t map, float _x, float _y, void(*callback)(TangramTouchItemArray)) {
    ((Tangram::Map*)map)->pickFeaturesAt(_x, _y, [=](const std::vector<Tangram::TouchItem>& features) {
        TangramPointerArray planeFeatures = { nullptr, 0 };
        planeFeatures.size = features.size();
        auto items = (TangramTouchItem*)malloc(planeFeatures.size * sizeof(TangramTouchItem));
        if (planeFeatures.size == 0) {
            free(items);
            planeFeatures.data = nullptr;
        } else {
            planeFeatures.data = items;
        }
        for (size_t i = 0; i < features.size(); ++i) {
            auto &feature = features[i];
            auto &item = items[i];
            item.distance = feature.distance;
            memcpy(item.position, feature.position, sizeof(feature.position));
            std::string jsonString = feature.properties->toJson();
            item.properties.size = jsonString.length();
            item.properties.data = malloc(jsonString.length());
            memcpy(item.properties.data, jsonString.data(), item.properties.size);
        }
        callback(planeFeatures);
    });
}

// Run this task asynchronously to Tangram's main update loop.
TANGRAM_CORE_EXPORT void tangramRunAsyncTask(tangram_map_t map, tangram_function_t _task) {
    return ((Tangram::Map*)map)->runAsyncTask(_task);
}

TANGRAM_CORE_EXPORT void tangramMapDestroy(tangram_map_t map_handle) {
    delete (Tangram::Map*)map_handle;
}

TANGRAM_CORE_EXPORT tangram_data_source_t tangramDataSourceCreate(const char* name, const char* url, int32_t maxzoom) {
    return new Tangram::ClientGeoJsonSource(name, url, maxzoom);
}

TANGRAM_CORE_EXPORT void tangramDataSourceAddGeoJSON(tangram_data_source_t source, const char*json, size_t json_length) {
  auto geoJsonSource = (Tangram::ClientGeoJsonSource*)source;
  if (json_length == SIZE_MAX) {
    geoJsonSource->addData(std::string(json));
  } else {
    geoJsonSource->addData(std::string(json, json_length));
  }
}

TANGRAM_CORE_EXPORT void tangramDataSourceRemoveFeature(tangram_data_source_t source, uint32_t featureId) {
  auto geoJsonSource = (Tangram::ClientGeoJsonSource*)source;
  geoJsonSource->removeFeature(featureId);
}

TANGRAM_CORE_EXPORT void tangramDataSourceSetAttribute(tangram_data_source_t source, const char*json, size_t json_length) {
  auto geoJsonSource = (Tangram::ClientGeoJsonSource*)source;
  if (json_length == SIZE_MAX) {
    geoJsonSource->addData(std::string(json));
  }
  else {
    geoJsonSource->addData(std::string(json, json_length));
  }
}

TANGRAM_CORE_EXPORT void tangramDataSourceDestroy(tangram_data_source_t source) {
    delete (Tangram::ClientGeoJsonSource*)source;
}

TANGRAM_CORE_EXPORT void tangramSetContinuousRendering(bool _isContinuous) {
    return setContinuousRendering(_isContinuous);
}

TANGRAM_CORE_EXPORT bool tangramIsContinuousRendering() {
    return isContinuousRendering();
}

TANGRAM_CORE_EXPORT void tangramSetDebugFlag(TangramDebugFlags _flag, bool _on) {
    return Tangram::setDebugFlag((Tangram::DebugFlags)_flag, _on);
}

// Get the boolean state of a debug feature (see debug.h)
TANGRAM_CORE_EXPORT bool tangramGetDebugFlag(TangramDebugFlags _flag) {
    return Tangram::getDebugFlag((Tangram::DebugFlags)_flag);
}

// Toggle the boolean state of a debug feature (see debug.h)
TANGRAM_CORE_EXPORT void tangramToggleDebugFlag(TangramDebugFlags _flag) {
    return Tangram::toggleDebugFlag((Tangram::DebugFlags)_flag);
}

}