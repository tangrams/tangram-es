#pragma once

#include "annotation/annotationRenderer.h"

#define GLFW_INCLUDE_GLEXT
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#include <cassert>
#include <string>

namespace Tangram {

class OverlayRenderer : public AnnotationRenderer {

public:
    OverlayRenderer() = default;

    ~OverlayRenderer() override;

    void initialize() override;

    void render(const AnnotationViewState& context) override;

    void deinitialize() override;

protected:
    GLuint mVertexBufferHandle = 0;
    GLuint mPositionAttributeLocation = 0;
    GLuint mShaderProgramHandle = 0;
    GLuint mFragmentShaderHandle = 0;
    GLuint mVertexShaderHandle = 0;

    const static char* vertexShaderSource;
    const static char* fragmentShaderSource;

    struct Vertex {
        float x, y;
    };
};

const char* OverlayRenderer::vertexShaderSource = R"RAW_GLSL(
attribute vec2 a_position;
varying vec4 v_color;
void main() {
    v_color = vec4(0., 1., 0., 1.);
    gl_Position = vec4(a_position.x, a_position.y, 0., 1.);
}
)RAW_GLSL";

const char* OverlayRenderer::fragmentShaderSource = R"RAW_GLSL(
varying vec4 v_color;
void main() {
    gl_FragColor = v_color;
}
)RAW_GLSL";

OverlayRenderer::~OverlayRenderer() {
    assert(mVertexBufferHandle == 0);
    assert(mShaderProgramHandle == 0);
    assert(mFragmentShaderHandle == 0);
    assert(mVertexShaderHandle == 0);
}

void OverlayRenderer::initialize() {
    // Set up shader program.
    mVertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(mVertexShaderHandle, 1, &vertexShaderSource, NULL);
    glCompileShader(mVertexShaderHandle);

    mFragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(mFragmentShaderHandle, 1, &fragmentShaderSource, NULL);
    glCompileShader(mFragmentShaderHandle);

    mShaderProgramHandle = glCreateProgram();
    glAttachShader(mShaderProgramHandle, mVertexShaderHandle);
    glAttachShader(mShaderProgramHandle, mFragmentShaderHandle);
    glLinkProgram(mShaderProgramHandle);

    mPositionAttributeLocation = glGetAttribLocation(mShaderProgramHandle, "a_position");

    // Set up vertex buffer.
    glGenBuffers(1, &mVertexBufferHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferHandle);
    glVertexAttribPointer(mPositionAttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, x));
    glEnableVertexAttribArray(mPositionAttributeLocation);

    Vertex vertices[] = {
        { -0.5, -0.5 },
        {  0.5, -0.5 },
        { -0.5,  0.5 },
        {  0.5, -0.5 },
        {  0.5,  0.5 },
        { -0.5,  0.5 },
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
}

void OverlayRenderer::render(const AnnotationViewState& context) {

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(mShaderProgramHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferHandle);
    glVertexAttribPointer(mPositionAttributeLocation, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, x));
    glEnableVertexAttribArray(mPositionAttributeLocation);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void OverlayRenderer::deinitialize() {
    // Tear down shader program.
    glDeleteProgram(mShaderProgramHandle);
    mShaderProgramHandle = 0;
    glDeleteShader(mVertexShaderHandle);
    mVertexShaderHandle = 0;
    glDeleteShader(mFragmentShaderHandle);
    mFragmentShaderHandle = 0;

    // Tear down vertex buffer.
    glDeleteBuffers(1, &mVertexBufferHandle);
    mVertexBufferHandle = 0;
}

} // namespace Tangram
