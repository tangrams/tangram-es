#define GLFW_INCLUDE_ES2

#include "tangram.h"
#include "platform.h"
#include "gl.h"

// Input handling
// ==============

const double double_tap_time = 0.5; // seconds

bool was_panning = false;
double last_mouse_up = -double_tap_time; // First click should never trigger a double tap
double last_x_down = 0.0;
double last_y_down = 0.0;

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
        last_x_down = x;
        last_y_down = y;
        return;
    }
    
    if (time - last_mouse_up < double_tap_time) {
        Tangram::handleDoubleTapGesture(x, y);
    } else {
        Tangram::handleTapGesture(x, y);
    }
    
    last_mouse_up = time;
    
}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {
    
    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    
    if (action == GLFW_PRESS) {
        
        if (was_panning) {
            Tangram::handlePanGesture(x - last_x_down, y - last_y_down);
        }
        
        was_panning = true;
        last_x_down = x;
        last_y_down = y;
    }
    
}

void scroll_callback(GLFWwindow* window, double scrollx, double scrolly) {
    
    static double scrolled = 0.0;
    scrolled += scrolly;
    
    // TODO: Update this for continuous zooming, using an arbitrary threshold for now
    if (scrolled * scrolled > 100.0) {
        
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        Tangram::handlePinchGesture(x, y, scrolled > 0 ? 1.5 : 0.5);
        scrolled = 0.0;
        
    }
}

// Window handling
// ===============

void window_size_callback(GLFWwindow* window, int width, int height) {
    
    Tangram::resize(width, height);
    
}

// Main program
// ============

int main(void) {

    GLFWwindow* window;
    int width = 800;
    int height = 600;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(width, height, "GLFW Window", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    Tangram::initialize();
    Tangram::resize(width, height);

    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    glfwSwapInterval(1);
    
    double lastTime = glfwGetTime();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {

        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
        lastTime = currentTime;
        
        /* Render here */
        Tangram::update(delta);
        Tangram::render();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }
    
    Tangram::teardown();
    glfwTerminate();
    return 0;
}
