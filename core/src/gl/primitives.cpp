#include "primitives.h"

#include "debug.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "gl/shaderProgram.h"
#include "gl/vertexLayout.h"
#include "platform.h"

namespace Tangram {

namespace Primitives {

static bool s_initialized;
static std::unique_ptr<ShaderProgram> s_shader;
static std::unique_ptr<VertexLayout> s_layout;
static glm::vec2 s_resolution;
static GLuint s_boundBuffer;
static GLboolean s_depthTest;

void init(glm::vec2 _resolution) {

    // lazy init
    if (!s_initialized) {
        std::string vert, frag;
        s_shader = std::unique_ptr<ShaderProgram>(new ShaderProgram());
        
        vert = stringFromResource("debugPrimitive.vs");
        frag = stringFromResource("debugPrimitive.fs");
        
        s_shader->setSourceStrings(frag, vert);
        s_shader->setUniformf("u_color", 1.f, 1.f, 1.f);

        s_layout = std::unique_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 2, GL_FLOAT, false, 0},
        }));

        s_initialized = true;
    }

    if (s_resolution != _resolution) {
        glm::mat4 proj = glm::ortho(0.f, _resolution.x, _resolution.y, 0.f, -1.f, 1.f);
        s_shader->setUniformMatrix4f("u_proj", glm::value_ptr(proj));
        s_resolution = _resolution;
    }
}

void saveState() {

    // save the current gl state
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*) &s_boundBuffer);
    glGetBooleanv(GL_DEPTH_TEST, &s_depthTest);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_DEPTH_TEST);
}

void popState() {

    // undo modification on the gl states
    glBindBuffer(GL_ARRAY_BUFFER, s_boundBuffer);

    if (s_depthTest) {
        glEnable(GL_DEPTH_TEST);
    }
}

void drawLine(const glm::vec2& _origin, const glm::vec2& _destination, glm::vec2 _resolution) {

    init(_resolution);

    glm::vec2 verts[2] = {
        glm::vec2(_origin.x, _origin.y),
        glm::vec2(_destination.x, _destination.y)
    };

    saveState();

    s_shader->use();

    // enable the layout for the line vertices
    s_layout->enable(*s_shader, 0, &verts);

    glDrawArrays(GL_LINES, 0, 2);
    popState();

}

void drawRect(const glm::vec2& _origin, const glm::vec2& _destination, glm::vec2 _resolution) {
    drawLine(_origin, {_destination.x, _origin.y}, _resolution);
    drawLine({_destination.x, _origin.y}, _destination, _resolution);
    drawLine(_destination, {_origin.x, _destination.y}, _resolution);
    drawLine({_origin.x,_destination.y}, _origin, _resolution);
}

void drawPoly(const glm::vec2* _polygon, size_t _n, glm::vec2 _resolution) {

    init(_resolution);

    saveState();

    s_shader->use();

    // enable the layout for the _polygon vertices
    s_layout->enable(*s_shader, 0, (void*)_polygon);

    glDrawArrays(GL_LINE_LOOP, 0, _n);
    popState();
}

}

}
