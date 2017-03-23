#include "glfwApp.h"
#include "data/clientGeoJsonSource.h"
#include "debug/textDisplay.h"
#include <GLFW/glfw3.h>

namespace Tangram {

namespace GlfwApp {

// Forward-declare our callback functions.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorMoveCallback(GLFWwindow* window, double x, double y);
void scrollCallback(GLFWwindow* window, double scrollx, double scrolly);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void dropCallback(GLFWwindow* window, int count, const char** paths);
void framebufferResizeCallback(GLFWwindow* window, int fWidth, int fHeight);

constexpr double double_tap_time = 0.5; // seconds
constexpr double scroll_span_multiplier = 0.05; // scaling for zoom and rotation
constexpr double scroll_distance_multiplier = 5.0; // scaling for shove
constexpr double single_tap_time = 0.25; //seconds (to avoid a long press being considered as a tap)

std::shared_ptr<Platform> platform;

std::string sceneFile;

std::string markerStylingPath = "layers.touch.point.draw.icons";
std::string polylineStyle = "{ style: lines, interactive: true, color: red, width: 20px, order: 5000 }";

GLFWwindow* main_window = nullptr;
Tangram::Map* map = nullptr;
int width = 800;
int height = 600;
float density = 1.0;
float pixel_scale = 1.0;
bool recreate_context = false;

bool was_panning = false;
double last_time_released = -double_tap_time; // First click should never trigger a double tap
double last_time_pressed = 0.0;
double last_time_moved = 0.0;
double last_x_down = 0.0;
double last_y_down = 0.0;
double last_x_velocity = 0.0;
double last_y_velocity = 0.0;

Tangram::MarkerID marker = 0;
Tangram::MarkerID poiMarker = 0;
Tangram::MarkerID polyline = 0;

bool keepRunning = true;

void create(std::shared_ptr<Platform> p, std::string f, int w, int h) {

    platform = p;
    sceneFile = f;
    width = w;
    height = h;

    if (!glfwInit()) {
        assert(false);
        return;
    }

    // Setup tangram
    if (!map) {
        map = new Tangram::Map(platform);
        map->loadSceneAsync(sceneFile.c_str(), true);
    }

    // Create a windowed mode window and its OpenGL context
    glfwWindowHint(GLFW_SAMPLES, 2);
    if (!main_window) {
        main_window = glfwCreateWindow(width, height, "Tangram ES", NULL, NULL);
    }
    if (!main_window) {
        glfwTerminate();
    }

    // Make the main_window's context current
    glfwMakeContextCurrent(main_window);

    // Set input callbacks
    glfwSetFramebufferSizeCallback(main_window, framebufferResizeCallback);
    glfwSetMouseButtonCallback(main_window, mouseButtonCallback);
    glfwSetCursorPosCallback(main_window, cursorMoveCallback);
    glfwSetScrollCallback(main_window, scrollCallback);
    glfwSetKeyCallback(main_window, keyCallback);
    glfwSetDropCallback(main_window, dropCallback);

    // Setup graphics
    map->setupGL();
    int fWidth = 0, fHeight = 0;
    glfwGetFramebufferSize(main_window, &fWidth, &fHeight);
    framebufferResizeCallback(main_window, fWidth, fHeight);

}

void run() {

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
        if (platform->isContinuousRendering()) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }
    }
}

void stop(int) {
    if (keepRunning) {
        logMsg("shutdown\n");
        keepRunning = false;
        glfwPostEmptyEvent();
    } else {
        logMsg("killed!\n");
        exit(1);
    }
}

void destroy() {
    if (map) {
        delete map;
        map = nullptr;
    }
    if (main_window) {
        glfwDestroyWindow(main_window);
        main_window = nullptr;
    }
    glfwTerminate();
}

template<typename T>
static constexpr T clamp(T val, T min, T max) {
    return val > max ? max : val < min ? min : val;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {

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
        Tangram::LngLat p;
        map->screenPositionToLngLat(x, y, &p.longitude, &p.latitude);
        map->setPositionEased(p.longitude, p.latitude, 1.f);

    } else if ((time - last_time_pressed) < single_tap_time) {
        // Single tap recognized
        Tangram::LngLat p;
        map->screenPositionToLngLat(x, y, &p.longitude, &p.latitude);


        if (!marker) {
            marker = map->markerAdd();

            map->markerSetStylingFromPath(marker, markerStylingPath.c_str());
            map->markerSetPoint(marker, p);
            map->markerSetDrawOrder(marker, mods);
            logMsg("Added Marker with zOrder: %d\n", mods);
        } else {
            static bool visible = true;
            map->markerSetPoint(marker, p);
            map->markerSetVisible(marker, visible);
            visible = !visible;
        }

        map->pickFeatureAt(x, y, [](const Tangram::FeaturePickResult* featurePickResult) {
            if (!featurePickResult) { return; }
            std::string name;
            if (featurePickResult->properties->getString("name", name)) {
                logMsg("Selected %s", name.c_str());
            }
        });

        map->pickLabelAt(x, y, [&](const Tangram::LabelPickResult* labelPickResult) {
            if (!labelPickResult) { return; }
            std::string type = labelPickResult->type == Tangram::LabelType::text ? "text" : "icon";
            std::string name;
            if (labelPickResult->touchItem.properties->getString("name", name)) {
                logMsg("Touched label %s %s", type.c_str(), name.c_str());
            }
            map->markerSetPoint(marker, labelPickResult->coordinates);
            map->markerSetVisible(marker, true);
        });

        map->pickMarkerAt(x, y, [](const Tangram::MarkerPickResult* markerPickResult) {
            if (!markerPickResult) { return; }
            logMsg("Selected marker id %d", markerPickResult->id);
        });

        static double last_x = 0, last_y = 0;

        if (!polyline) {
            polyline = map->markerAdd();
            map->markerSetStylingFromString(polyline, polylineStyle.c_str());
        }

        if (last_x != 0) {
            Tangram::LngLat coords[2];
            map->screenPositionToLngLat(x, y, &coords[0].longitude, &coords[0].latitude);
            map->screenPositionToLngLat(last_x, last_y, &coords[1].longitude, &coords[1].latitude);

            map->markerSetPolyline(polyline, coords, 2);
        }

        last_x = x;
        last_y = y;

        platform->requestRender();
    }

    last_time_released = time;

}

void cursorMoveCallback(GLFWwindow* window, double x, double y) {

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

}

void scrollCallback(GLFWwindow* window, double scrollx, double scrolly) {

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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

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
            case GLFW_KEY_9:
                Tangram::toggleDebugFlag(Tangram::DebugFlags::selection_buffer);
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
                    map->queueSceneUpdate("sources.osm.type", "GeoJSON");
                    map->queueSceneUpdate("sources.osm.url", "https://tile.mapzen.com/mapzen/vector/v1/all/{z}/{x}/{y}.json");
                } else {
                    map->queueSceneUpdate("sources.osm.type", "MVT");
                    map->queueSceneUpdate("sources.osm.url", "https://tile.mapzen.com/mapzen/vector/v1/all/{z}/{x}/{y}.mvt");
                }
                geoJSON = !geoJSON;
                map->applySceneUpdates();
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, true);
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

void dropCallback(GLFWwindow* window, int count, const char** paths) {

    sceneFile = std::string(paths[0]);
    map->loadSceneAsync(sceneFile.c_str());

}

void framebufferResizeCallback(GLFWwindow* window, int fWidth, int fHeight) {

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

} // namespace GlfwApp

} // namespace Tangram
