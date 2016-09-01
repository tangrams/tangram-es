#include "tangram.h"
#include "platform_osx.h"
#include "data/clientGeoJsonSource.h"
#include "debug/textDisplay.h"
#include <cmath>
#include <memory>
#include <signal.h>
#include <stdlib.h>

// Forward declaration
void init_main_window(bool recreate);

std::string sceneFile = "scene.yaml";

std::string markerStyling = "{ style: 'lines', color: 'purple', width: 10px, order: 100 }";

GLFWwindow* main_window = nullptr;
Tangram::Map* map = nullptr;
int width = 800;
int height = 600;
float density = 1.0;
bool recreate_context = false;
float pixel_scale = 1.0;

// Input handling
// ==============

const double double_tap_time = 0.5; // seconds
const double scroll_span_multiplier = 0.05; // scaling for zoom and rotation
const double scroll_distance_multiplier = 5.0; // scaling for shove
const double single_tap_time = 0.25; //seconds (to avoid a long press being considered as a tap)

bool was_panning = false;
double last_time_released = -double_tap_time; // First click should never trigger a double tap
double last_time_pressed = 0.0;
double last_time_moved = 0.0;
double last_x_down = 0.0;
double last_y_down = 0.0;
double last_x_velocity = 0.0;
double last_y_velocity = 0.0;

using namespace Tangram;

std::shared_ptr<ClientGeoJsonSource> data_source;
LngLat last_point;
std::vector<LngLat> taps;
MarkerID marker = 0;

template<typename T>
static constexpr T clamp(T val, T min, T max) {
    return val > max ? max : val < min ? min : val;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    if (button != GLFW_MOUSE_BUTTON_1) {
        return; // This event is for a mouse button that we don't care about
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    x *= density;
    y *= density;
    double time = glfwGetTime();

    if (was_panning && action == GLFW_RELEASE) {
        was_panning = false;
        auto vx = clamp(last_x_velocity, -2000.0, 2000.0);
        auto vy = clamp(last_y_velocity, -2000.0, 2000.0);
        map->handleFlingGesture(x, y, vx, vy);
        return; // Clicks with movement don't count as taps, so stop here
    }

    if (action == GLFW_PRESS) {
        map->handlePanGesture(0.0f, 0.0f, 0.0f, 0.0f);
        last_x_down = x;
        last_y_down = y;
        last_time_pressed = time;
        return;
    }

    if ((time - last_time_released) < double_tap_time) {
        // Double tap recognized
        LngLat p;
        map->screenPositionToLngLat(x, y, &p.longitude, &p.latitude);
        map->setPositionEased(p.longitude, p.latitude, 1.f);

        logMsg("pick feature\n");
        map->clearDataSource(*data_source, true, true);

        auto picks = map->pickFeaturesAt(x, y);
        std::string name;
        logMsg("picked %d features\n", picks.size());
        for (const auto& it : picks) {
            if (it.properties->getString("name", name)) {
                logMsg(" - %f\t %s\n", it.distance, name.c_str());
            }
        }
    } else if ((time - last_time_pressed) < single_tap_time) {
        // Single tap recognized
        LngLat p1;
        map->screenPositionToLngLat(x, y, &p1.longitude, &p1.latitude);
        taps.push_back(p1);

        if (!(last_point == LngLat{0, 0})) {
            LngLat p2 = last_point;
            logMsg("add line (%f, %f)  (%f, %f)\n",
                   p1.longitude, p1.latitude,
                   p2.longitude, p2.latitude);

            if (!marker) {
                marker = map->markerAdd();
                map->markerSetStyling(marker, markerStyling.c_str());
            }
            map->markerSetPolyline(marker, taps.data(), taps.size());
        }
        last_point = p1;

        // Tangram::clearDataSource(*data_source, false, true);
        // This updates the tiles (maybe we need a recalcTiles())
        requestRender();
    }

    last_time_released = time;

}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {

    x *= density;
    y *= density;

    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    double time = glfwGetTime();

    if (action == GLFW_PRESS) {

        if (was_panning) {
            map->handlePanGesture(last_x_down, last_y_down, x, y);
        }

        was_panning = true;
        last_x_velocity = (x - last_x_down) / (time - last_time_moved);
        last_y_velocity = (y - last_y_down) / (time - last_time_moved);
        last_x_down = x;
        last_y_down = y;

    }

    last_time_moved = time;

    unsigned int pixel = map->readSelectionBufferAt(x, y);
    LOGS("pixel(%1.f,%1.f):%u", x, y, pixel);

}

void scroll_callback(GLFWwindow* window, double scrollx, double scrolly) {

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    x *= density;
    y *= density;

    bool rotating = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
    bool shoving = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    if (shoving) {
        map->handleShoveGesture(scroll_distance_multiplier * scrolly);
    } else if (rotating) {
        map->handleRotateGesture(x, y, scroll_span_multiplier * scrolly);
    } else {
        map->handlePinchGesture(x, y, 1.0 + scroll_span_multiplier * scrolly, 0.f);
    }

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_1:
                Tangram::toggleDebugFlag(Tangram::DebugFlags::freeze_tiles);
                break;
            case GLFW_KEY_2:
                Tangram::toggleDebugFlag(Tangram::DebugFlags::proxy_colors);
                break;
            case GLFW_KEY_3:
                Tangram::toggleDebugFlag(Tangram::DebugFlags::tile_bounds);
                break;
            case GLFW_KEY_4:
                Tangram::toggleDebugFlag(Tangram::DebugFlags::tile_infos);
                break;
            case GLFW_KEY_5:
                Tangram::toggleDebugFlag(Tangram::DebugFlags::labels);
                break;
            case GLFW_KEY_6:
                Tangram::toggleDebugFlag(Tangram::DebugFlags::draw_all_labels);
                break;
            case GLFW_KEY_7:
                Tangram::toggleDebugFlag(Tangram::DebugFlags::tangram_infos);
                break;
            case GLFW_KEY_8:
                Tangram::toggleDebugFlag(Tangram::DebugFlags::tangram_stats);
                break;
            case GLFW_KEY_BACKSPACE:
                recreate_context = true;
                break;
            case GLFW_KEY_R:
                map->loadSceneAsync(sceneFile.c_str());
                break;
            case GLFW_KEY_Z:
                map->setZoomEased(map->getZoom() + 1.f, 1.5f);
                break;
            case GLFW_KEY_N:
                map->setRotationEased(0.f, 1.f);
                break;
            case GLFW_KEY_S:
                if (pixel_scale == 1.0) {
                    pixel_scale = 2.0;
                } else if (pixel_scale == 2.0) {
                    pixel_scale = 0.75;
                } else {
                    pixel_scale = 1.0;
                }
                map->loadSceneAsync(sceneFile.c_str());
                map->setPixelScale(pixel_scale);
                break;
            case GLFW_KEY_P:
                map->queueSceneUpdate("cameras", "{ main_camera: { type: perspective } }");
                map->applySceneUpdates();
                break;
            case GLFW_KEY_I:
                map->queueSceneUpdate("cameras", "{ main_camera: { type: isometric } }");
                map->applySceneUpdates();
                break;
            case GLFW_KEY_G:
                static bool geoJSON = false;
                if (!geoJSON) {
                    LOGS("Switching to GeoJSON data source");
                    map->queueSceneUpdate("sources.osm.type", "GeoJSON");
                    map->queueSceneUpdate("sources.osm.url", "https://vector.mapzen.com/osm/all/{z}/{x}/{y}.json");
                } else {
                    LOGS("Switching to MVT data source");
                    map->queueSceneUpdate("sources.osm.type", "MVT");
                    map->queueSceneUpdate("sources.osm.url", "https://vector.mapzen.com/osm/all/{z}/{x}/{y}.mvt");
                }
                geoJSON = !geoJSON;
                map->applySceneUpdates();
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(main_window, true);
                break;
            case GLFW_KEY_F1:
                map->setPosition(-74.00976419448854, 40.70532700869127);
                map->setZoom(16);
                break;
            case GLFW_KEY_F2:
                map->setPosition(8.82, 53.08);
                map->setZoom(14);
                break;
        default:
                break;
        }
    }
}

void drop_callback(GLFWwindow* window, int count, const char** paths) {

    sceneFile = std::string(paths[0]);
    map->loadSceneAsync(sceneFile.c_str());

}

// Window handling
// ===============

void framebuffer_size_callback(GLFWwindow* window, int fWidth, int fHeight) {

    int wWidth = 0, wHeight = 0;
    glfwGetWindowSize(main_window, &wWidth, &wHeight);
    float new_density = (float)fWidth / (float)wWidth;
    if (new_density != density) {
        recreate_context = true;
        density = new_density;
    }
    map->setPixelScale(density);
    map->resize(fWidth, fHeight);

}

void init_main_window(bool recreate) {

    // Setup tangram
    if (!map) {
        map = new Tangram::Map();
        map->loadSceneAsync(sceneFile.c_str(), true);
    }

    if (!recreate) {
        // Destroy old window
        if (main_window != nullptr) {
            glfwDestroyWindow(main_window);
        }

        // Create a windowed mode window and its OpenGL context
        glfwWindowHint(GLFW_SAMPLES, 2);
        main_window = glfwCreateWindow(width, height, "Tangram ES", NULL, NULL);
        if (!main_window) {
            glfwTerminate();
        }

        // Make the main_window's context current
        glfwMakeContextCurrent(main_window);

        // Set input callbacks
        glfwSetFramebufferSizeCallback(main_window, framebuffer_size_callback);
        glfwSetMouseButtonCallback(main_window, mouse_button_callback);
        glfwSetCursorPosCallback(main_window, cursor_pos_callback);
        glfwSetScrollCallback(main_window, scroll_callback);
        glfwSetKeyCallback(main_window, key_callback);
        glfwSetDropCallback(main_window, drop_callback);
    }

    // Setup graphics
    map->setupGL();
    int fWidth = 0, fHeight = 0;
    glfwGetFramebufferSize(main_window, &fWidth, &fHeight);
    framebuffer_size_callback(main_window, fWidth, fHeight);

    data_source = std::make_shared<ClientGeoJsonSource>("touch", "");
    map->addDataSource(data_source);

}

// Main program
// ============

int main(int argc, char* argv[]) {

    static bool keepRunning = true;

    // Give it a chance to shutdown cleanly on CTRL-C
    signal(SIGINT, [](int) {
            if (keepRunning) {
                logMsg("shutdown\n");
                keepRunning = false;
                glfwPostEmptyEvent();
            } else {
                logMsg("killed!\n");
                exit(1);
            }});

    int argi = 0;
    while (++argi < argc) {
        if (strcmp(argv[argi - 1], "-f") == 0) {
            sceneFile = std::string(argv[argi]);
            logMsg("File from command line: %s\n", argv[argi]);
            break;
        }
    }

    // Initialize networking
    NSurlInit();

    // Initialize the windowing library
    if (!glfwInit()) {
        return -1;
    }

    init_main_window(false);

    double lastTime = glfwGetTime();

    // Loop until the user closes the window
    while (keepRunning && !glfwWindowShouldClose(main_window)) {

        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
        lastTime = currentTime;

        // Render
        map->update(delta);
        map->render();

        // Swap front and back buffers
        glfwSwapBuffers(main_window);

        // Poll for and process events
        if (isContinuousRendering()) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }

        if (recreate_context) {
            logMsg("recreate context\n");
             // Simulate GL context loss
            init_main_window(true);
            recreate_context = false;
        }

    }

    finishUrlRequests();

    if (map) {
        delete map;
        map = nullptr;
    }

    glfwTerminate();
    return 0;
}
