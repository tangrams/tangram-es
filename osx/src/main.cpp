#define GLFW_INCLUDE_ES2

#include "tangram.h"
#include "platform.h"
#include "gl.h"

// Input handling
// ==============

bool was_panning = false;

void window_size_callback(GLFWwindow* window, int width, int height) {
    
    Tangram::resize(width, height);

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    
    if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE) {
        
        if (!was_panning) {
            
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            Tangram::handleTapGesture(x, y);
            
        } else {
            
            was_panning = false;
            
        }
        
    }
}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {
    
    static double last_x = 0.0;
    static double last_y = 0.0;
    
    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    
    if (action == GLFW_PRESS) {
        
        if (!was_panning) {
            
            last_x = x;
            last_y = y;
            was_panning = true;
            
        }

        Tangram::handlePanGesture(x - last_x, y - last_y);
        
        last_x = x;
        last_y = y;
        
    }
    
}

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
