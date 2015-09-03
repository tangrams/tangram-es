#include "debugStyle.h"

#include "tangram.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"

#include <vector>
#include <memory>
#include <string>

namespace Tangram {

DebugStyle::DebugStyle(std::string _name, Blending _blendMode, GLenum _drawMode) : Style(_name, _blendMode, _drawMode) {
}

void DebugStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0}
    }));

}

void DebugStyle::constructShaderProgram() {

    std::string vertShaderSrcStr = stringFromResource("debug.vs");
    std::string fragShaderSrcStr = stringFromResource("debug.fs");

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

}

void DebugStyle::onBeginBuildTile(Tile &_tile) const {

    if (Tangram::getDebugFlag(Tangram::DebugFlags::tile_bounds)) {

        Mesh* mesh = new Mesh(m_vertexLayout, m_drawMode);

        // Add four vertices to draw the outline of the tile in red

        std::vector<PosColVertex> vertices;

        GLuint abgr = 0xff0000ff;

        vertices.reserve(4);
        vertices.push_back({{ -1.f, -1.f, 0.f }, abgr });
        vertices.push_back({{  1.f, -1.f, 0.f }, abgr });
        vertices.push_back({{  1.f,  1.f, 0.f }, abgr });
        vertices.push_back({{ -1.f,  1.f, 0.f }, abgr });

        mesh->addVertices(std::move(vertices), { 0, 1, 2, 3, 0 });

        _tile.getMesh(*this).reset(mesh);

    }
}

}
