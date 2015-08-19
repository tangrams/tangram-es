#include "polygonStyle.h"

#include "tangram.h"
#include "util/builders.h"
#include "gl/shaderProgram.h"
#include "tile/tile.h"
#include "glm/gtx/normal.hpp"

#include <cmath>

namespace Tangram {

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

PolygonStyle::Parameters PolygonStyle::parseRule(const DrawRule& _rule) const {
    Parameters p;
    _rule.get(StyleParamKey::color, p.color);
    _rule.get(StyleParamKey::order, p.order);
    _rule.get(StyleParamKey::extrude, p.extrude);

    return p;
}

void PolygonStyle::buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const {

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

    auto& mesh = static_cast<PolygonStyle::Mesh&>(_mesh);

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


void PolygonStyle::addVertex(glm::vec3 p, glm::vec3 n, GLuint abgr, float layer,
                   std::vector<int>& indices,
                   std::vector<PolygonVertex>& vertices) const {
  auto id = vertices.size();
  vertices.push_back({ p, n, glm::vec2(0), abgr, layer });
  indices.push_back(id);
}

void PolygonStyle::buildMesh(const std::vector<uint16_t>& indices,
                             const std::vector<Point>& points,
                             const DrawRule& _rule,
                             const Properties& _props,
                             VboMesh& _mesh, Tile& _tile) const {
    GLuint abgr = 0xffe6f0f2;
    std::vector<PolygonVertex> vertices;
    GLfloat layer = 1;

    std::vector<int> newIndices;
    vertices.reserve(indices.size() * 3);

    for (size_t i = 0; i < indices.size(); i += 3) {
        auto p1 = points[indices[i + 0]];
        auto p2 = points[indices[i + 1]];
        auto p3 = points[indices[i + 2]];

        auto a = p2 - p1;
        auto b = p3 - p1;

        auto c = glm::cross(a, b);
        auto n = glm::normalize(glm::vec3(0,0,0.25)) - glm::normalize(c);
        //auto n =  glm::normalize(c);

        addVertex(p1, n, abgr, layer, newIndices, vertices);
        addVertex(p3, n, abgr, layer, newIndices, vertices);
        addVertex(p2, n, abgr, layer, newIndices, vertices);
    }

    auto& mesh = static_cast<PolygonStyle::Mesh&>(_mesh);
    mesh.addVertices(std::move(vertices), std::move(newIndices));
}

}
