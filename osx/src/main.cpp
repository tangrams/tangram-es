#include "tangram.h"
#include "platform.h"
#include "gl.h"
#include <cmath>

// Forward declaration
void init_main_window();

const char* sceneFile = "scene.yaml";

// Input handling
// ==============

const double double_tap_time = 0.5; // seconds
const double scroll_multiplier = 0.05; // scaling for zoom
const double single_tap_time = 0.25; //seconds (to avoid a long press being considered as a tap)

bool was_panning = false;
double last_mouse_up = -double_tap_time; // First click should never trigger a double tap
double last_mouse_down = 0.0f;
double last_x_down = 0.0;
double last_y_down = 0.0;
int data_source_id = -1;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    if (button != GLFW_MOUSE_BUTTON_1) {
        return; // This event is for a mouse button that we don't care about
    }

    if (was_panning) {
        was_panning = false;
        return; // Clicks with movement don't count as taps
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    double time = glfwGetTime();

    if (action == GLFW_PRESS) {
        Tangram::handlePanGesture(0.0f, 0.0f, 0.0f, 0.0f);
        last_x_down = x;
        last_y_down = y;
        last_mouse_down = glfwGetTime();
        return;
    }

    if (time - last_mouse_up < double_tap_time) {
        Tangram::clearSourceData(data_source_id);
        // Tangram::handleDoubleTapGesture(x, y);
    } else if ( (time - last_mouse_down) < single_tap_time) {
        double coords[4] = { x, y, 0, 0 };
        Tangram::screenToWorldCoordinates(coords[0], coords[1]);
        Tangram::screenToWorldCoordinates(coords[2], coords[3]);
        Tangram::addSourceLine(data_source_id, coords, 2);
        // Tangram::handleTapGesture(x, y);
    }

    last_mouse_up = time;

}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {

    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);

    if (action == GLFW_PRESS) {

        if (was_panning) {
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

    bool rotating = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
    bool shoving = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

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
            case GLFW_KEY_BACKSPACE:
                init_main_window(); // Simulate GL context loss
                break;
            case GLFW_KEY_R:
                Tangram::loadScene(sceneFile);
                break;
        default:
                break;
        }
    }
}

// Window handling
// ===============

GLFWwindow* main_window = nullptr;
int width = 800;
int height = 600;

void window_size_callback(GLFWwindow* window, int width, int height) {

    Tangram::resize(width, height);

}

void init_main_window() {

    // Setup tangram
    Tangram::initialize(sceneFile);

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
    
    // Setup graphics
    Tangram::setupGL();
    Tangram::resize(width, height);

    // Work-around for a bug in GLFW on retina displays
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(main_window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    data_source_id = Tangram::addDataSource("touch");

}

// Main program
// ============

int main(void) {

    // Initialize the windowing library
    if (!glfwInit()) {
        return -1;
    }

    init_main_window();

    // Initialize networking
    NSurlInit();

    double lastTime = glfwGetTime();

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(main_window)) {

        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
        lastTime = currentTime;

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
    }

    glfwTerminate();
    return 0;
}
