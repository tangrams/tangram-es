#include "polygonStyle.h"

#include "tangram.h"
#include "platform.h"
#include "material.h"
#include "util/builders.h"
#include "util/extrude.h"
#include "gl/shaderProgram.h"
#include "gl/typedMesh.h"
#include "tile/tile.h"
#include "scene/drawRule.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/gtc/type_precision.hpp"

#include <cmath>

#define POSITION_SCALE 4096.0f
#define TEXTURE_SCALE 16384.0f
#define NORMAL_SCALE 127.0f

namespace Tangram {

struct PolygonVertex {
    glm::i16vec4 pos; // pos.w contains layer (params.order)
    glm::i8vec4 norm;
    glm::i16vec2 texcoord;
    GLuint abgr;
};

using Mesh = TypedMesh<PolygonVertex>;


PolygonStyle::PolygonStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode) {
}

void PolygonStyle::constructVertexLayout() {

    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 4, GL_SHORT, false, 0},
        {"a_normal", 4, GL_BYTE, true, 0},
        {"a_texcoord", 2, GL_SHORT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
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
    _rule.get(StyleParamKey::order, p.order);

    return p;
}

void PolygonStyle::buildPolygon(const Polygon& _polygon, const DrawRule& _rule,
                                const Properties& _props, VboMesh& _mesh, Tile& _tile) const {

    std::vector<PolygonVertex> vertices;

    Parameters params = parseRule(_rule);

    GLuint abgr = params.color;
    auto& extrude = params.extrude;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr = abgr << (_tile.getID().z % 6);
    }

    const static std::string key_height("height");
    const static std::string key_min_height("min_height");

    PolygonBuilder builder = {
        [&](const glm::vec3& _coord, const glm::vec3& _normal, const glm::vec2& _uv) {
            glm::vec3 coord = _coord * POSITION_SCALE;
            glm::i16vec2 uv = _uv * TEXTURE_SCALE;
            glm::i8vec4 normal = glm::i8vec4{ _normal * NORMAL_SCALE, 0};

            vertices.push_back({ glm::i16vec4{coord, params.order}, normal, uv, abgr });
        },
        [&](size_t sizeHint){ vertices.reserve(sizeHint); }
    };

    auto& mesh = static_cast<Mesh&>(_mesh);

    float tileUnitsPerMeter = _tile.getInverseScale();
    float minHeight = getLowerExtrudeMeters(extrude, _props) * tileUnitsPerMeter;
    float height = getUpperExtrudeMeters(extrude, _props) * tileUnitsPerMeter;

    if (minHeight != height) {

        Builders::buildPolygonExtrusion(_polygon, minHeight, height, builder);
        mesh.addVertices(std::move(vertices), std::move(builder.indices));

        // TODO add builder.clear() ?;
        builder.numVertices = 0;
    }

    Builders::buildPolygon(_polygon, height, builder);
    mesh.addVertices(std::move(vertices), std::move(builder.indices));
}

}
