#include "context.h"
#include "map.h"
#include "log.h"
#include "rpiPlatform.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <termios.h>

#define KEY_ESC      27     // esc
#define KEY_ZOOM_IN  61     // =
#define KEY_ZOOM_OUT 45     // -
#define KEY_UP       119    // w
#define KEY_LEFT     97     // a
#define KEY_DOWN     115    // s
#define KEY_RIGHT    100    // d

using namespace Tangram;

Map* map = nullptr;
std::shared_ptr<RpiPlatform> platform;

std::string mapzenApiKey;

static bool bUpdate = true;

struct LaunchOptions {
    std::string sceneFilePath = "scene.yaml";
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    float zoom = 0.0f;
    float rotation = 0.0f;
    float tilt = 0.0f;
};

LaunchOptions getLaunchOptions(int argc, char **argv) {

    LaunchOptions options;

    for (int i = 1; i < argc - 1; i++) {
        std::string argName = argv[i];
        std::string argValue = argv[i + 1];
        if (argName == "-s" || argName == "--scene") {
            options.sceneFilePath = argValue;
        } else if (argName == "-lat" ) {
            options.latitude = std::stod(argValue);
        } else if (argName == "-lon" ) {
            options.longitude = std::stod(argValue);
        } else if (argName == "-z" || argName == "--zoom" ) {
            options.zoom = std::stof(argValue);
        } else if (argName == "-x" || argName == "--x_position") {
            options.x = std::stoi(argValue);
        } else if (argName == "-y" || argName == "--y_position") {
            options.y = std::stoi(argValue);
        } else if (argName == "-w" || argName == "--width") {
            options.width = std::stoi(argValue);
        } else if (argName == "-h" || argName == "--height") {
            options.height = std::stoi(argValue);
        } else if (argName == "-t" || argName == "--tilt") {
            options.tilt = std::stof(argValue);
        } else if (argName == "-r" || argName == "--rotation") {
            options.rotation = std::stof(argValue);
        }
    }
    return options;
}

struct Timer {
    timeval start;

    Timer() {
        gettimeofday(&start, NULL);
    }

    // Get the time in seconds since delta was last called.
    double deltaSeconds() {
        timeval current;
        gettimeofday(&current, NULL);
        double delta = (double)(current.tv_sec - start.tv_sec) + (current.tv_usec - start.tv_usec) * 1.0E-6;
        start = current;
        return delta;
    }
};

int main(int argc, char **argv) {

    printf("Starting an interactive map window. Use keys to navigate:\n"
           "\t'%c' = Pan up\n"
           "\t'%c' = Pan left\n"
           "\t'%c' = Pan down\n"
           "\t'%c' = Pan right\n"
           "\t'%c' = Zoom in\n"
           "\t'%c' = Zoom out\n"
           "\t'esc'= Exit\n"
           "Press 'enter' to continue.",
           KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT, KEY_ZOOM_IN, KEY_ZOOM_OUT);

    if (getchar() == -1) {
        return 1;
    }

    LaunchOptions options = getLaunchOptions(argc, argv);

    UrlClient::Options urlClientOptions;
    urlClientOptions.numberOfThreads = 4;

    platform = std::make_shared<RpiPlatform>(urlClientOptions);

    // Start OpenGL context
    createSurface(options.x, options.y, options.width, options.height);

    // Get Mapzen API key from environment variables.
    char* mapzenApiKeyEnvVar = getenv("MAPZEN_API_KEY");
    if (mapzenApiKeyEnvVar && strlen(mapzenApiKeyEnvVar) > 0) {
        mapzenApiKey = mapzenApiKeyEnvVar;
    } else {
        LOGW("No API key found!\n\nMapzen data sources require an API key. "
             "Sign up for a free key at http://mapzen.com/developers and then set it from the command line with: "
             "\n\n\texport MAPZEN_API_KEY=YOUR_KEY_HERE\n");
    }

    map = new Map(platform);
    if (mapzenApiKey.empty()) {
        map->loadScene(options.sceneFilePath.c_str(), false, {}, nullptr);
    } else {
        map->loadScene(options.sceneFilePath.c_str(), false, { SceneUpdate("global.sdk_mapzen_api_key", mapzenApiKey) }, nullptr);
    }

    map->setupGL();
    map->resize(getWindowWidth(), getWindowHeight());
    map->setPosition(options.longitude, options.latitude);
    map->setZoom(options.zoom);
    map->setTilt(options.tilt);
    map->setRotation(options.rotation);

    // Start clock
    Timer timer;

    while (bUpdate) {
        pollInput();
        double dt = timer.deltaSeconds();
        if (getRenderRequest() || platform->isContinuousRendering() ) {
            setRenderRequest(false);
            map->update(dt);
            map->render();
            swapSurface();
        }
    }

    if (map) {
        delete map;
        map = nullptr;
    }

    destroySurface();
    return 0;
}

//======================================================================= EVENTS

void onKeyPress(int _key) {
    switch (_key) {
        case KEY_ZOOM_IN:
            map->setZoom(map->getZoom() + 1.0f);
            break;
        case KEY_ZOOM_OUT:
            map->setZoom(map->getZoom() - 1.0f);
            break;
        case KEY_UP:
            map->handlePanGesture(0.0, 0.0, 0.0, 100.0);
            break;
        case KEY_DOWN:
            map->handlePanGesture(0.0, 0.0, 0.0, -100.0);
            break;
        case KEY_LEFT:
            map->handlePanGesture(0.0, 0.0, 100.0, 0.0);
            break;
        case KEY_RIGHT:
            map->handlePanGesture(0.0, 0.0, -100.0, 0.0);
            break;
        case KEY_ESC:
            bUpdate = false;
            break;
        default:
            logMsg(" -> %i\n",_key);
    }
    platform->requestRender();
}

void onMouseMove(float _x, float _y) {
    platform->requestRender();
}

void onMouseClick(float _x, float _y, int _button) {
    platform->requestRender();
}

void onMouseDrag(float _x, float _y, int _button) {
    if( _button == 1 ){

        map->handlePanGesture(_x - getMouseVelX(), _y + getMouseVelY(), _x, _y);

    } else if( _button == 2 ){
        if ( getKeyPressed() == 'r') {
            float scale = -0.05;
            float rot = atan2(getMouseVelY(),getMouseVelX());
            if( _x < getWindowWidth()/2.0 ) {
                scale *= -1.0;
            }
            map->handleRotateGesture(getWindowWidth()/2.0, getWindowHeight()/2.0, rot*scale);
        } else if ( getKeyPressed() == 't') {
            map->handleShoveGesture(getMouseVelY()*0.005);
        } else {
            map->handlePinchGesture(getWindowWidth()/2.0, getWindowHeight()/2.0, 1.0 + getMouseVelY()*0.001, 0.f);
        }

    }
    platform->requestRender();
}

void onMouseRelease(float _x, float _y) {
    platform->requestRender();
}

void onViewportResize(int _newWidth, int _newHeight) {
    platform->requestRender();
}
