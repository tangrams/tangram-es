#include "debugStyle.h"

#include "tangram.h"
#include "material.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/mesh.h"

#include <vector>
#include <memory>
#include <string>

static const char* debug_vs = R"(
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
attribute vec4 a_position;
attribute vec4 a_color;
varying vec4 v_color;

void main() {
    v_color = a_color;
    gl_Position = u_proj * u_view * u_model * a_position;
}
)";

static const char* debug_fs = R"(
varying vec4 v_color;

void main() {
    gl_FragColor = v_color;
}
)";

namespace Tangram {

struct PosColVertex {
    // Position Data
    glm::vec3 pos;
    // Color Data
    GLuint abgr;
};


DebugStyle::DebugStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode, false) {}

void DebugStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));

}

void DebugStyle::buildFragmentShaderSource(ShaderSource& out) {
    out << debug_fs;
}

void DebugStyle::buildVertexShaderSource(ShaderSource& out, bool _selectionPass) {
    out << debug_vs;
}

struct DebugStyleBuilder : public StyleBuilder {

    const DebugStyle& m_style;

    void setup(const Tile& _tile) override {}
    void setup(const Marker& _marker, int zoom) override {}

    std::unique_ptr<StyledMesh> build() override {
        if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_bounds)) {
            return nullptr;
        }

        auto mesh = std::make_unique<Mesh<PosColVertex>>(m_style.vertexLayout(),
                                                         m_style.drawMode());

        GLuint abgr = 0xff0000ff;

        // Add four vertices to draw the outline of the tile in red
        mesh->compile({{ 0, 1, 2, 3, 0 },
                       {{{ 0.f, 0.f, 0.f }, abgr },
                        {{ 1.f, 0.f, 0.f }, abgr },
                        {{ 1.f, 1.f, 0.f }, abgr },
                        {{ 0.f, 1.f, 0.f }, abgr }}});

        return std::move(mesh);
    }

    const Style& style() const override { return m_style; }

    DebugStyleBuilder(const DebugStyle& _style) : m_style(_style) {}

};

std::unique_ptr<StyleBuilder> DebugStyle::createBuilder() const {
    return std::make_unique<DebugStyleBuilder>(*this);
}

}
