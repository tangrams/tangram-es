#define GLFW_INCLUDE_ES2

#include "tangram.h"
#include "platform.h"
#include "gl.h"

// Input handling
// ==============

const double double_tap_time = 0.5; // seconds

bool was_panning = false;
double last_mouse_up = -double_tap_time;

void window_size_callback(GLFWwindow* window, int width, int height) {
    
    Tangram::resize(width, height);

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    
    if (button != GLFW_MOUSE_BUTTON_1 || action != GLFW_RELEASE) {
        return; // Stop here because this is a mouse event that we don't care about
    }
    
    if (was_panning) {
        
        was_panning = false;
        return; // Stop here so this isn't counted as the first tap in a double-tap
        
    }
    
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    double time = glfwGetTime();
    
    if (time - last_mouse_up < double_tap_time) {
        
        Tangram::handleDoubleTapGesture(x, y);
        
    } else {
        
        Tangram::handleTapGesture(x, y);
        
    }
    
    last_mouse_up = time;
    
}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {
    
    static double last_x = 0.0;
    static double last_y = 0.0;
    
    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    
    if (action == GLFW_PRESS) {
        
        if (!was_panning) {
            
            // TODO: This ignores the first frame of panning movement; to correct this we
            // would need to track the cursor position whenever the mouse button is pressed
            was_panning = true;
            
        } else {
            
            Tangram::handlePanGesture(x - last_x, y - last_y);
            
        }
        
        last_x = x;
        last_y = y;
        
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
