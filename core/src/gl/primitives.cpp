#include "primitives.h"

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "gl/error.h"
#include "gl/shaderProgram.h"
#include "gl/vertexLayout.h"
#include "gl/renderState.h"
#include "gl/texture.h"
#include "log.h"

#include "debugPrimitive_vs.h"
#include "debugPrimitive_fs.h"

#include "debugTexture_vs.h"
#include "debugTexture_fs.h"

namespace Tangram {

namespace Primitives {

static bool s_initialized;
static std::unique_ptr<ShaderProgram> s_shader;
static std::unique_ptr<VertexLayout> s_layout;

static UniformLocation s_uColor{"u_color"};
static UniformLocation s_uProj{"u_proj"};


static std::unique_ptr<ShaderProgram> s_textureShader;
static std::unique_ptr<VertexLayout> s_textureLayout;

static UniformLocation s_uTextureProj{"u_proj"};

void init() {

    // lazy init
    if (!s_initialized) {
        s_shader = std::make_unique<ShaderProgram>();

        s_shader->setShaderSource(SHADER_SOURCE(debugPrimitive_vs),
                                  SHADER_SOURCE(debugPrimitive_fs));

        s_layout = std::unique_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 2, GL_FLOAT, false, 0},
        }));


        s_textureShader = std::make_unique<ShaderProgram>();

        s_textureShader->setShaderSource(SHADER_SOURCE(debugTexture_vs),
                                         SHADER_SOURCE(debugTexture_fs));

        s_textureLayout = std::unique_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 2, GL_FLOAT, false, 0},
            {"a_uv", 2, GL_FLOAT, false, 0},
        }));


        s_initialized = true;
        GL::lineWidth(1.5f);
    }
}

void deinit() {

    s_shader.reset(nullptr);
    s_layout.reset(nullptr);
    s_textureShader.reset(nullptr);
    s_textureLayout.reset(nullptr);
    s_initialized = false;

}

void drawLine(RenderState& rs, const glm::vec2& _origin, const glm::vec2& _destination) {

    init();

    glm::vec2 verts[2] = {
        glm::vec2(_origin.x, _origin.y),
        glm::vec2(_destination.x, _destination.y)
    };

    if (!s_shader->use(rs)) { return; }

    GLint boundBuffer;
    GL::getIntegerv(GL_ARRAY_BUFFER_BINDING, &boundBuffer);
    rs.vertexBuffer(0);
    rs.depthTest(GL_FALSE);

    // enable the layout for the line vertices
    s_layout->enable(rs, *s_shader, 0, &verts);

    GL::drawArrays(GL_LINES, 0, 2);

    rs.vertexBuffer(boundBuffer);
}

void drawRect(RenderState& rs, const glm::vec2& _origin, const glm::vec2& _destination) {
    drawLine(rs, _origin, {_destination.x, _origin.y});
    drawLine(rs, {_destination.x, _origin.y}, _destination);
    drawLine(rs, _destination, {_origin.x, _destination.y});
    drawLine(rs, {_origin.x,_destination.y}, _origin);
}

void drawPoly(RenderState& rs, const glm::vec2* _polygon, size_t _n) {
    init();

    if (!s_shader->use(rs)) { return; }

    GLint boundBuffer;
    GL::getIntegerv(GL_ARRAY_BUFFER_BINDING, &boundBuffer);
    rs.vertexBuffer(0);
    rs.depthTest(GL_FALSE);

    // enable the layout for the _polygon vertices
    s_layout->enable(rs, *s_shader, 0, (void*)_polygon);

    GL::drawArrays(GL_LINE_LOOP, 0, _n);

    rs.vertexBuffer(boundBuffer);
}

void drawTexture(RenderState& rs, Texture& _tex, glm::vec2 _pos, glm::vec2 _dim) {
    init();

    if (!s_textureShader->use(rs)) { return; }

    GLint boundBuffer;
    GL::getIntegerv(GL_ARRAY_BUFFER_BINDING, &boundBuffer);
    rs.vertexBuffer(0);
    rs.depthTest(GL_FALSE);

    float w = _tex.getWidth();
    float h = _tex.getHeight();

    if (_dim != glm::vec2(0)) {
        w = _dim.x;
        h = _dim.y;
    }
    glm::vec4 vertices[6] = {
        {_pos.x, _pos.y, 0, 1},
        {_pos.x, _pos.y + h, 0, 0},
        {_pos.x + w, _pos.y, 1, 1},

        {_pos.x + w, _pos.y, 1, 1},
        {_pos.x, _pos.y + h, 0, 0},
        {_pos.x + w, _pos.y + h, 1, 0},
    };

    _tex.bind(rs, 0);

    // enable the layout for the _polygon vertices
    s_textureLayout->enable(rs, *s_textureShader, 0, (void*)vertices);

    GL::drawArrays(GL_TRIANGLES, 0, 6);

    rs.vertexBuffer(boundBuffer);
}

void setColor(RenderState& rs, unsigned int _color) {
    init();

    float r = (_color >> 16 & 0xff) / 255.0;
    float g = (_color >> 8  & 0xff) / 255.0;
    float b = (_color       & 0xff) / 255.0;

    s_shader->setUniformf(rs, s_uColor, r, g, b);
}

void setResolution(RenderState& rs, float _width, float _height) {
    init();

    glm::mat4 proj = glm::ortho(0.f, _width, _height, 0.f, -1.f, 1.f);
    s_shader->setUniformMatrix4f(rs, s_uProj, proj);
    s_textureShader->setUniformMatrix4f(rs, s_uTextureProj, proj);
}

}

}
