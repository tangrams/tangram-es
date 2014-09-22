#include "tangram.h"

const GLfloat vertices[] = {
    0.0f, 1.0f,
    -1.0f, 0.0f,
    1.0f, 0.0f
};

ShaderProgram simpleShader;
GLuint vbo;
ViewModule *view = new ViewModule();
float t;

const std::string vertShaderSrc =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "attribute vec4 a_position;\n"
    "void main() {\n"
    "  gl_Position = a_position;\n"
    "}\n";

const std::string fragShaderSrc =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "void main(void) {\n"
    "  gl_FragColor = vec4(93.0/255.0, 141.0/255.0, 148.0/255.0, 1.0);\n"
    "}\n";



void initializeOpenGL()
{

    // Make a VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Make a shader program
    simpleShader.buildFromSourceStrings(fragShaderSrc, vertShaderSrc);

    t = 0;

    logMsg("%s\n", "initialize");

}

void resizeViewport(int newWidth, int newHeight)
{
    glViewport(0, 0, newWidth, newHeight);

    view->setAspect(newWidth, newHeight);

    logMsg("%s\n", "resizeViewport");
}

void renderFrame()
{

    // Draw a triangle!
    t += 0.016;
    float sintsqr = pow(sin(t), 2);
    glClearColor(0.8f * sintsqr, 0.32f * sintsqr, 0.3f * sintsqr, 1.0f);
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    simpleShader.use();
    GLint posAttrib = simpleShader.getAttribLocation("a_position");
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posAttrib);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    GLenum glError = glGetError();
    if (glError) {
        logMsg("%s\n", "glError!!");
    }

}
