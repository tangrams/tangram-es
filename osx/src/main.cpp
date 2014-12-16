#define GLFW_INCLUDE_ES2

#include "tangram.h"
#include "platform.h"
#include "gl.h"

void window_size_callback(GLFWwindow* window, int width, int height) {
    
    Tangram::resize(width, height);

}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    
    if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        
        Tangram::handleTapGesture(x/width, -y/height);
    }
}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {
    
    static double last_x = 0.0;
    static double last_y = 0.0;
    static double last_t = 0.0;
    
    int action = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    
    if (action == GLFW_PRESS) {
        
        double t = glfwGetTime();
        
        if (last_t > 0.0) {
            float vx = (x - last_x)/(t - last_t);
            float vy = (y - last_y)/(t - last_t);
            if (vx > 1000 || vx < -1000 || vy > 1000 || vy < -1000) {
                logMsg("WHOOOPS");
                // dt here is way less than 16ms, probably getting several events per frame which... is bad?
            }
            Tangram::handlePanGesture(vx, vy);
        }
        
        last_x = x;
        last_y = y;
        last_t = t;
        
    } else {
        
        
        last_t = -1.0;
        
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
