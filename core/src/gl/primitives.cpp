#include "primitives.h"

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "gl/shaderProgram.h"
#include "gl/vertexLayout.h"
#include "gl/renderState.h"
#include "platform.h"

#include "shaders/debugPrimitive_vs.h"
#include "shaders/debugPrimitive_fs.h"

namespace Tangram {

namespace Primitives {

static bool s_initialized;
static std::unique_ptr<ShaderProgram> s_shader;
static std::unique_ptr<VertexLayout> s_layout;
static glm::vec2 s_resolution;
static GLuint s_boundBuffer;

static UniformLocation s_uColor{"u_color"};
static UniformLocation s_uProj{"u_proj"};

void init() {

    // lazy init
    if (!s_initialized) {
        s_shader = std::unique_ptr<ShaderProgram>(new ShaderProgram());

        s_shader->setSourceStrings(SHADER_SOURCE(debugPrimitive_fs),
                                   SHADER_SOURCE(debugPrimitive_vs));

        s_layout = std::unique_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 2, GL_FLOAT, false, 0},
        }));

        s_initialized = true;
        GL_CHECK(glLineWidth(1.5f));
    }
}

void saveState() {
    // save the current gl state
    GL_CHECK(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*) &s_boundBuffer));
    RenderState::depthTest(GL_FALSE);
    RenderState::vertexBuffer(0);
}

void popState() {
    // undo modification on the gl states
    RenderState::vertexBuffer(s_boundBuffer);
}

void drawLine(const glm::vec2& _origin, const glm::vec2& _destination) {

    init();

    glm::vec2 verts[2] = {
        glm::vec2(_origin.x, _origin.y),
        glm::vec2(_destination.x, _destination.y)
    };

    saveState();

    s_shader->use();

    // enable the layout for the line vertices
    s_layout->enable(*s_shader, 0, &verts);

    GL_CHECK(glDrawArrays(GL_LINES, 0, 2));
    popState();

}

void drawRect(const glm::vec2& _origin, const glm::vec2& _destination) {
    drawLine(_origin, {_destination.x, _origin.y});
    drawLine({_destination.x, _origin.y}, _destination);
    drawLine(_destination, {_origin.x, _destination.y});
    drawLine({_origin.x,_destination.y}, _origin);
}

void drawPoly(const glm::vec2* _polygon, size_t _n) {
    init();

    saveState();

    s_shader->use();

    // enable the layout for the _polygon vertices
    s_layout->enable(*s_shader, 0, (void*)_polygon);

    GL_CHECK(glDrawArrays(GL_LINE_LOOP, 0, _n));
    popState();
}

void setColor(unsigned int _color) {
    init();

    float r = (_color >> 16 & 0xff) / 255.0;
    float g = (_color >> 8  & 0xff) / 255.0;
    float b = (_color       & 0xff) / 255.0;

    s_shader->setUniformf(s_uColor, r, g, b);
}

void setResolution(float _width, float _height) {
    init();

    glm::mat4 proj = glm::ortho(0.f, _width, _height, 0.f, -1.f, 1.f);
    s_shader->setUniformMatrix4f(s_uProj, proj);
}

}

}
