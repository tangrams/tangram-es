#include "polygonStyle.h"

#include "tangram.h"
#include "platform.h"
#include "material.h"
#include "util/builders.h"
#include "gl/shaderProgram.h"
#include "gl/typedMesh.h"
#include "tile/tile.h"
#include "scene/drawRule.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include <cmath>

namespace Tangram {

struct PolygonVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 texcoord;
    GLuint abgr;
    GLfloat layer;
};

using Mesh = TypedMesh<PolygonVertex>;


PolygonStyle::PolygonStyle(std::string _name, Blending _blendMode, GLenum _drawMode) : Style(_name, _blendMode, _drawMode) {
}

void PolygonStyle::constructVertexLayout() {

    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_texcoord", 2, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_layer", 1, GL_FLOAT, false, 0}
    }));

}

void PolygonStyle::constructShaderProgram() {

    std::string vertShaderSrcStr = stringFromFile("shaders/polygon.vs", PathType::internal);
    std::string fragShaderSrcStr = stringFromFile("shaders/polygon.fs", PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

VboMesh* PolygonStyle::newMesh() const {
    return new Mesh(m_vertexLayout, m_drawMode);
}

PolygonStyle::Parameters PolygonStyle::parseRule(const DrawRule& _rule) const {
    Parameters p;
    _rule.get(StyleParamKey::color, p.color);
    _rule.get(StyleParamKey::extrude, p.extrude);
    if (!_rule.get(StyleParamKey::order, p.order)) {
        LOGW("No 'order' specified for feature, ordering cannot be guaranteed :(");
    }

    return p;
}

void PolygonStyle::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {

    if (!_rule.contains(StyleParamKey::color)) {
        const auto& blocks = m_shaderProgram->getSourceBlocks();
        if (blocks.find("color") == blocks.end() && blocks.find("filter") == blocks.end()) {
            return; // No color parameter or color block? NO SOUP FOR YOU
        }
    }

    std::vector<PolygonVertex> vertices;

    Parameters params = parseRule(_rule);

    GLuint abgr = params.color;
    GLfloat layer = params.order;
    auto& extrude = params.extrude;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr = abgr << (_tile.getID().z % 6);
    }

    const static std::string key_height("height");
    const static std::string key_min_height("min_height");

    PolygonBuilder builder = {
        [&](const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv){
            vertices.push_back({ coord, normal, uv, abgr, layer });
        },
        [&](size_t sizeHint){ vertices.reserve(sizeHint); }
    };

    auto& mesh = static_cast<Mesh&>(_mesh);

    float height = 0.0f, minHeight = 0.0f;

    if (extrude[0] != 0.0f || extrude[1] != 0.0f) {

        height = _props.getNumeric(key_height) * _tile.getInverseScale();
        minHeight = _props.getNumeric(key_min_height) * _tile.getInverseScale();

        if (std::isnan(extrude[1])) {
            if (!std::isnan(extrude[0])) {
                height = extrude[0];
            }
        } else {
            minHeight = extrude[0];
            height = extrude[1];
        }

        Builders::buildPolygonExtrusion(_polygon, minHeight, height, builder);
        mesh.addVertices(std::move(vertices), std::move(builder.indices));

        // TODO add builder.clear() ?;
        builder.numVertices = 0;
    }

    Builders::buildPolygon(_polygon, height, builder);
    mesh.addVertices(std::move(vertices), std::move(builder.indices));
}

}
