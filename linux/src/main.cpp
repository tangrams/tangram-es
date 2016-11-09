#include <curl/curl.h>
#include <memory>

#include "tangram.h"
#include "data/clientGeoJsonSource.h"
#include "debug/textDisplay.h"
#include "platform_linux.h"
#include "log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <signal.h>

#include <GLFW/glfw3.h>

using namespace Tangram;

// Forward declaration
void init_main_window(bool recreate);

std::string sceneFile = "scene.yaml";
std::string markerStyling = "{ style: 'points', color: 'white', size: [25px, 25px], order: 100, collide: false }";

GLFWwindow* main_window = nullptr;
Tangram::Map* map = nullptr;
int width = 800;
int height = 600;
bool recreate_context;
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
bool scene_editing_mode = false;
MarkerID marker = 0;

bool testClientDataSource = true;
std::shared_ptr<ClientGeoJsonSource> data_source;
LngLat last_point;

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
    double time = glfwGetTime();

    if (was_panning) {
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

        LngLat p;
        map->screenPositionToLngLat(x, y, &p.longitude, &p.latitude);
        // map->setPositionEased(p.longitude, p.latitude, 1.f);

        LOG("pick feature");

        if (testClientDataSource) {
            map->clearTileSource(*data_source, true, true);
        }

        map->pickFeaturesAt(x, y, [](const auto& items) {
            for (const auto& item : items) {
                LOG("%s", item.properties->toJson().c_str());

                std::string name = "noname";
                item.properties->getString("name", name);
                LOGS("%s", name.c_str());
            }
        });
    } else if ((time - last_time_pressed) < single_tap_time) {
        LngLat p;
        map->screenPositionToLngLat(x, y, &p.longitude, &p.latitude);

        if (testClientDataSource) {
            LOG("Added point %f,%f", p.longitude, p.latitude);

            Properties prop;
            prop.set("type", "point");
            data_source->addPoint(prop, p);

            if (!(last_point == LngLat{0, 0})) {
                LngLat p2 = last_point;
                Properties prop1;
                prop1.set("type", "line");
                data_source->addLine(prop1, {p, p2});
            }
            last_point = p;
        } else {
            marker = map->markerAdd();
            map->markerSetStyling(marker, markerStyling.c_str());
            map->markerSetPoint(marker, p);
            map->markerSetDrawOrder(marker, mods);
            LOG("Added marker with zOrder: %d", mods);
        }
        // This updates the tiles (maybe we need a recalcTiles())
        requestRender();
    }

    last_time_released = time;

}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {

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

void scroll_callback(GLFWwindow* window, double scrollx, double scrolly) {

    double x, y;
    glfwGetCursorPos(window, &x, &y);

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
            case GLFW_KEY_9:
                Tangram::toggleDebugFlag(Tangram::DebugFlags::selection_buffer);
                break;
            case GLFW_KEY_R:
                map->loadSceneAsync(sceneFile.c_str());
                break;
            case GLFW_KEY_E:
                if (scene_editing_mode) {
                    scene_editing_mode = false;
                    setContinuousRendering(false);
                    glfwSwapInterval(0);
                } else {
                    scene_editing_mode = true;
                    setContinuousRendering(true);
                    glfwSwapInterval(1);
                }
                map->loadSceneAsync(sceneFile.c_str());
                break;
            case GLFW_KEY_BACKSPACE:
                recreate_context = true;
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

void window_size_callback(GLFWwindow* window, int width, int height) {

    map->resize(width, height);

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
        glfwSetWindowSizeCallback(main_window, window_size_callback);
        glfwSetMouseButtonCallback(main_window, mouse_button_callback);
        glfwSetCursorPosCallback(main_window, cursor_pos_callback);
        glfwSetScrollCallback(main_window, scroll_callback);
        glfwSetKeyCallback(main_window, key_callback);
        glfwSetDropCallback(main_window, drop_callback);
    }

    // Setup graphics
    map->setupGL();
    map->resize(width, height);

    if (testClientDataSource) {
        data_source = std::make_shared<ClientGeoJsonSource>("touch", "");
        map->addTileSource(data_source);
    }
}


// Main program
// ============

int main(int argc, char* argv[]) {

    static bool keepRunning = true;

    // Give it a chance to shutdown cleanly on CTRL-C
    signal(SIGINT, [](int) {
            if (keepRunning) {
                LOG("shutdown");
                keepRunning = false;
                glfwPostEmptyEvent();
            } else {
                LOG("killed!");
                exit(1);
            }});

    int argi = 0;
    while (++argi < argc) {
        if (strcmp(argv[argi - 1], "-f") == 0) {
            sceneFile = std::string(argv[argi]);
            LOG("File from command line: %s", argv[argi]);
            break;
        }
    }

    // Initialize the windowing library
    if (!glfwInit()) {
        return -1;
    }

    struct stat sb;
    //if (stat(sceneFile.c_str(), &sb) == -1) {
        //LOG("scene file not found!");
        //exit(EXIT_FAILURE);
    //}
    auto last_mod = sb.st_mtime;

    init_main_window(false);

    // Initialize cURL
    curl_global_init(CURL_GLOBAL_DEFAULT);

    double lastTime = glfwGetTime();

    setContinuousRendering(false);
    glfwSwapInterval(0);

    recreate_context = false;

    // Loop until the user closes the window
    while (keepRunning && !glfwWindowShouldClose(main_window)) {

        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
        lastTime = currentTime;

        processNetworkQueue();

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
            LOG("recreate context");
             // Simulate GL context loss
            init_main_window(true);
            recreate_context = false;
        }

        if (scene_editing_mode) {
            //if (stat(sceneFile.c_str(), &sb) == 0) {
                if (last_mod != sb.st_mtime) {
                    map->loadSceneAsync(sceneFile.c_str());
                    last_mod = sb.st_mtime;
                }
            //}
        }
    }

    finishUrlRequests();

    if (map) {
        delete map;
        map = nullptr;
    }

    curl_global_cleanup();
    glfwTerminate();
    return 0;
}
