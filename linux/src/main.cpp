#include <curl/curl.h>
#include <memory>

#include "tangram.h"
#include "data/clientGeoJsonSource.h"
#include "platform_linux.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>

using namespace Tangram;

// Forward declaration
void init_main_window();

std::string sceneFile = "scene.yaml";

GLFWwindow* main_window = nullptr;
int width = 800;
int height = 600;

// Input handling
// ==============

const double double_tap_time = 0.5; // seconds
const double scroll_multiplier = 0.05; // scaling for zoom
const double single_tap_time = 0.25; //seconds (to avoid a long press being considered as a tap)

bool was_panning = false;
bool is_zooming = false;
bool is_rotating = false;
double last_mouse_up = -double_tap_time; // First click should never trigger a double tap
double last_mouse_down = 0.0f;
double last_x_down = 0.0;
double last_y_down = 0.0;
bool scene_editing_mode = false;

std::shared_ptr<ClientGeoJsonSource> data_source;
LngLat last_point;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    if (button != GLFW_MOUSE_BUTTON_1) {
        return; // This event is for a mouse button that we don't care about
    }

    if (was_panning) {
        was_panning = false;
        is_zooming = false;
        is_rotating = false;
        return; // Clicks with movement don't count as taps
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    double time = glfwGetTime();

    if ((mods & GLFW_MOD_SHIFT) != 0) { is_zooming = true; }
    if ((mods & GLFW_MOD_CONTROL) != 0) { is_rotating = true; }

    if (action == GLFW_PRESS) {
        Tangram::handlePanGesture(0.0f, 0.0f, 0.0f, 0.0f);
        last_x_down = x;
        last_y_down = y;
        last_mouse_down = glfwGetTime();
        return;
    }

    if (time - last_mouse_up < double_tap_time) {

        LngLat p { x, y };
        Tangram::screenToWorldCoordinates(p.longitude, p.latitude);
        Tangram::setPosition(p.longitude, p.latitude, 1.f);

        logMsg("pick feature\n");
        Tangram::clearDataSource(*data_source, true, true);

        auto picks = Tangram::pickFeaturesAt(x, y);
        std::string name;
        logMsg("picked %d features\n", picks.size());
        for (const auto& it : picks) {
            if (it.properties->getString("name", name)) {
                logMsg(" - %f\t %s\n", it.distance, name.c_str());
            }
        }
    } else if ( (time - last_mouse_down) < single_tap_time) {
        LngLat p1 {x, y};
        Tangram::screenToWorldCoordinates(p1.longitude, p1.latitude);

        if (!(last_point == LngLat{0, 0})) {
            LngLat p2 = last_point;

            logMsg("add line %f %f - %f %f\n",
                   p1.longitude, p1.latitude,
                   p2.longitude, p2.latitude);

            // Let's make variant public!
            // data_source->addLine(Properties{{"type", "line" }}, {p1, p2});
            // data_source->addPoint(Properties{{"type", "point" }}, p2);
            Properties prop1;
            prop1.add("type", "line");
            data_source->addLine(prop1, {p1, p2});
            Properties prop2;
            prop2.add("type", "point");
            data_source->addPoint(prop2, p2);
        }
        last_point = p1;
        Tangram::clearDataSource(*data_source, false, true);
    }

    last_mouse_up = time;

}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {

    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);

    if (action == GLFW_PRESS) {

        if (is_zooming) {
            Tangram::handlePinchGesture(width/2, height/2, 1.0 + 0.01 * (last_y_down - y), 0.f);

        } else if (is_rotating) {
            Tangram::handleRotateGesture(width/2, height/2, 0.01 * (last_x_down - x));

        } else if (was_panning) {
            Tangram::handlePanGesture(last_x_down, last_y_down, x, y);
        }

        was_panning = true;
        last_x_down = x;
        last_y_down = y;
    }

}

void scroll_callback(GLFWwindow* window, double scrollx, double scrolly) {

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    bool rotating = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
    bool shoving = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                   glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    if (shoving) {
        Tangram::handleShoveGesture(scroll_multiplier * scrolly);
    } else if (rotating) {
        Tangram::handleRotateGesture(x, y, scroll_multiplier * scrolly);
    } else {
        Tangram::handlePinchGesture(x, y, 1.0 + scroll_multiplier * scrolly, 0.f);
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
            case GLFW_KEY_R:
                Tangram::loadScene(sceneFile.c_str());
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
                Tangram::loadScene(sceneFile.c_str());
                break;
            case GLFW_KEY_BACKSPACE:
                init_main_window(); // Simulate GL context loss
                break;
            case GLFW_KEY_N:
                Tangram::setRotation(0.f, 1.f);
                break;
            default:
                break;
        }
    }
}

void drop_callback(GLFWwindow* window, int count, const char** paths) {

    sceneFile = std::string(paths[0]);
    Tangram::loadScene(sceneFile.c_str());

}

// Window handling
// ===============

void window_size_callback(GLFWwindow* window, int w, int h) {
    width = w;
    height = h;
    Tangram::resize(width, height);
}

void init_main_window() {

    // Setup tangram
    Tangram::initialize(sceneFile.c_str());

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

    // Setup graphics
    Tangram::setupGL();
    Tangram::resize(width, height);

    data_source = std::make_shared<ClientGeoJsonSource>("touch", "");
    Tangram::addDataSource(data_source);
}

// Main program
// ============

int main(int argc, char* argv[]) {

    int argi = 0;
    while (++argi < argc) {
        if (strcmp(argv[argi - 1], "-f") == 0) {
            sceneFile = std::string(argv[argi]);
            logMsg("File from command line: %s\n", argv[argi]);
            break;
        }
    }

    // Initialize the windowing library
    if (!glfwInit()) {
        return -1;
    }

    struct stat sb;
    if (stat(sceneFile.c_str(), &sb) == -1) {
        logMsg("scene file not found!");
        exit(EXIT_FAILURE);
    }
    auto last_mod = sb.st_mtime;

    init_main_window();

    // Initialize cURL
    curl_global_init(CURL_GLOBAL_DEFAULT);

    double lastTime = glfwGetTime();

    setContinuousRendering(false);
    glfwSwapInterval(0);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(main_window)) {

        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
        lastTime = currentTime;

        processNetworkQueue();

        // Render
        Tangram::update(delta);
        Tangram::render();

        // Swap front and back buffers
        glfwSwapBuffers(main_window);

        // Poll for and process events
        if (isContinuousRendering()) {
            glfwPollEvents();
        } else {
            glfwWaitEvents();
        }
        if (scene_editing_mode) {
            if (stat(sceneFile.c_str(), &sb) == 0) {
                if (last_mod != sb.st_mtime) {
                    Tangram::loadScene(sceneFile.c_str());
                    last_mod = sb.st_mtime;
                }
            }
        }
    }

    curl_global_cleanup();
    glfwTerminate();
    return 0;
}
