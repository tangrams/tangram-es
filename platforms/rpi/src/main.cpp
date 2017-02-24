#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>

#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <curl/curl.h>

#include "context.h"
#include "tangram.h"
#include "platform_linux.h"

#include <iostream>
#include "glm/trigonometric.hpp"

#define KEY_ESC      113    // q
#define KEY_ZOOM_IN  45     // -
#define KEY_ZOOM_OUT 61     // =
#define KEY_UP       119    // w
#define KEY_LEFT     97     // a
#define KEY_RIGHT    115    // s
#define KEY_DOWN     122    // z

struct timeval tv;
unsigned long long timePrev, timeStart;

Tangram::Map* map = nullptr;
std::shared_ptr<Tangram::LinuxPlatform> platform;

static bool bUpdate = true;

//==============================================================================
void setup(int argc, char **argv);
void newFrame();

int main(int argc, char **argv){

    UrlClient::Environment urlClientEnvironment;

    UrlClient::Options urlClientOptions;
    urlClientOptions.numberOfThreads = 4;

    platform = std::make_shared<Tangram::LinuxPlatform>(urlClientOptions);

    // Start OpenGL context
    initGL(argc, argv);

    /* Do Curl Init */
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Set background color and clear buffers
    setup(argc, argv);

    // Start clock
    gettimeofday(&tv, NULL);
    timeStart = timePrev = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

    while (bUpdate) {
        updateGL();
        if (getRenderRequest() || platform->isContinuousRendering() ) {
            setRenderRequest(false);
            newFrame();
        }
    }

    if (map) {
        delete map;
        map = nullptr;
    }

    curl_global_cleanup();
    closeGL();
    return 0;
}

void setup(int argc, char **argv) {
    int width = getWindowWidth();
    int height = getWindowHeight();
    float rot = 0.0f;
    float zoom = 0.0f;
    float tilt = 0.0f;
    double lat = 0.0f;
    double lon = 0.0f;
    std::string scene = "scene.yaml";

    for (int i = 1; i < argc - 1; i++) {
        std::string argName(argv[i]), argValue(argv[i + 1]);
        if (argName == "-s" || argName == "--scene") {
            scene = argValue;
        } else if (argName == "-lat" ) {
            lat = std::stod(argValue);
        } else if (argName == "-lon" ) {
            lon = std::stod(argValue);
        } else if (argName == "-z" || argName == "--zoom" ) {
            zoom = std::stof(argValue);
        } else if (argName == "-w" || argName == "--width") {
            width = std::stoi(argValue);
        } else if (argName == "-h" || argName == "--height") {
            height = std::stoi(argValue);
        } else if (argName == "-t" || argName == "--tilt") {
            tilt = std::stof(argValue);
        } else if (argName == "-r" || argName == "--rotation") {
            rot = std::stof(argValue);
        }
    }

    map = new Tangram::Map(platform);
    map->loadSceneAsync(scene.c_str());
    map->setupGL();
    map->resize(width, height);
    if (lon != 0.0f && lat != 0.0f) {
        map->setPosition(lon,lat);
    }
    if (zoom != 0.0f) {
        map->setZoom(zoom);
    }
    if (tilt != 0.0f) {
        map->setTilt(glm::radians(tilt));
    }
    if (rot != 0.0f) {
        map->setRotation(glm::radians(rot));
    }
}

void newFrame() {

    // Update
    gettimeofday( &tv, NULL);
    unsigned long long timeNow = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    double delta = ((double)timeNow - (double)timePrev)*0.001;

    //logMsg("New frame (delta %d msec)\n",delta);

    map->update(delta);
    timePrev = timeNow;

    // Render
    map->render();

    renderGL();
}

//======================================================================= EVENTS

void onKeyPress(int _key) {
    switch (_key) {
        case KEY_ZOOM_IN:
            map->handlePinchGesture(0.0,0.0,0.5,0.0);
            break;
        case KEY_ZOOM_OUT:
            map->handlePinchGesture(0.0,0.0,2.0,0.0);
            break;
        case KEY_UP:
            map->handlePanGesture(0.0,0.0,0.0,100.0);
            break;
        case KEY_DOWN:
            map->handlePanGesture(0.0,0.0,0.0,-100.0);
            break;
        case KEY_LEFT:
            map->handlePanGesture(0.0,0.0,100.0,0.0);
            break;
        case KEY_RIGHT:
            map->handlePanGesture(0.0,0.0,-100.0,0.0);
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
