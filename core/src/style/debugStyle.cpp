#include "debugStyle.h"

#include "tangram.h"
#include "platform.h"
#include "material.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/typedMesh.h"

#include <vector>
#include <memory>
#include <string>

namespace Tangram {

struct PosColVertex {
    // Position Data
    glm::vec3 pos;
    // Color Data
    GLuint abgr;
};

using Mesh = TypedMesh<PosColVertex>;


DebugStyle::DebugStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode) {}

void DebugStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));

}

void DebugStyle::constructShaderProgram() {

    std::string vertShaderSrcStr = stringFromFile("shaders/debug.vs", PathType::internal);
    std::string fragShaderSrcStr = stringFromFile("shaders/debug.fs", PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

}

namespace {
struct Builder : public StyleBuilder {

    const DebugStyle& m_style;

    void begin(const Tile& _tile) override {}

    std::unique_ptr<VboMesh> build() override {
        if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_bounds)) {
            return nullptr;
        }

        auto mesh = std::make_unique<Mesh>(m_style.vertexLayout(), m_style.drawMode());

        // Add four vertices to draw the outline of the tile in red
        std::vector<PosColVertex> vertices;

        GLuint abgr = 0xff0000ff;

        vertices.reserve(4);
        vertices.push_back({{ 0.f, 0.f, 0.f }, abgr });
        vertices.push_back({{ 1.f, 0.f, 0.f }, abgr });
        vertices.push_back({{ 1.f, 1.f, 0.f }, abgr });
        vertices.push_back({{ 0.f, 1.f, 0.f }, abgr });

        mesh->addVertices(std::move(vertices), { 0, 1, 2, 3, 0 });
        mesh->compileVertexBuffer();

        return std::move(mesh);
    }

    const Style& style() const override { return m_style; }

    Builder(const DebugStyle& _style) : StyleBuilder(_style), m_style(_style) {}

};
}

std::unique_ptr<StyleBuilder> DebugStyle::createBuilder() const {
    return std::make_unique<Builder>(*this);
}

}
