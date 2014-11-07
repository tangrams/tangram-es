#define GLFW_INCLUDE_ES2

#include "tangram.h"
#include "platform.h"
#include "gl.h"

void window_size_callback(GLFWwindow* window, int width, int height)
{
    Tangram::resize(width, height);
}

int main(void)
{
    GLFWwindow* window;
    int width = 800;
    int height = 600;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(width, height, "GLFW Window", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    Tangram::initialize();
    Tangram::resize(width, height);

    glfwSetWindowSizeCallback(window, window_size_callback);
    
    double lastTime = glfwGetTime();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
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
