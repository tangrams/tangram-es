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

constexpr float position_scale = 1024.0f;
constexpr float texture_scale = 65535.0f;
constexpr float normal_scale = 127.0f;

namespace Tangram {

struct PolygonVertex {

    PolygonVertex(glm::vec3 position, uint32_t order,
                  glm::vec3 normal, glm::vec2 uv, GLuint abgr)
        : pos(glm::i16vec4{ position * position_scale, order }),
          norm(normal * normal_scale),
          texcoord(uv * texture_scale),
          abgr(abgr) {}

    glm::i16vec4 pos; // pos.w contains layer (params.order)
    glm::i8vec3 norm;
    uint8_t padding = 0;
    glm::u16vec2 texcoord;
    GLuint abgr;
};

using Mesh = TypedMesh<PolygonVertex>;


PolygonStyle::PolygonStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode) {}

void PolygonStyle::constructVertexLayout() {

    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 4, GL_SHORT, false, 0},
        {"a_normal", 4, GL_BYTE, true, 0}, // The 4th byte is for padding
        {"a_texcoord", 2, GL_UNSIGNED_SHORT, true, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
    }));

}

void PolygonStyle::constructShaderProgram() {

    std::string vertShaderSrcStr = stringFromFile("shaders/polygon.vs", PathType::internal);
    std::string fragShaderSrcStr = stringFromFile("shaders/polygon.fs", PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

namespace {

struct Builder : public StyleBuilder {

    const PolygonStyle& m_style;

    std::unique_ptr<Mesh> m_mesh;
    float m_tileUnitsPerMeter;
    int m_zoom;

    struct Parameters {
        uint32_t order = 0;
        uint32_t color = 0xff00ffff;
        glm::vec2 extrude;
    };

    void begin(const Tile& _tile) override {
        m_tileUnitsPerMeter = _tile.getInverseScale();
        m_zoom = _tile.getID().z;
        m_mesh = std::make_unique<Mesh>(m_style.vertexLayout(), m_style.drawMode());
    }

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;

    std::unique_ptr<VboMesh> build() override { return std::move(m_mesh); };


    Builder(const PolygonStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    Parameters parseRule(const DrawRule& _rule) const;
};

Builder::Parameters Builder::parseRule(const DrawRule& _rule) const {
    Parameters p;
    _rule.get(StyleParamKey::color, p.color);
    _rule.get(StyleParamKey::extrude, p.extrude);
    _rule.get(StyleParamKey::order, p.order);

    return p;
}

void Builder::addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) {

    std::vector<PolygonVertex> vertices;

    Parameters params = parseRule(_rule);

    GLuint abgr = params.color;
    auto& extrude = params.extrude;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr = abgr << (m_zoom % 6);
    }

    PolygonBuilder builder = {
        [&](const glm::vec3& _coord, const glm::vec3& _normal, const glm::vec2& _uv) {
            vertices.push_back({ _coord, params.order, _normal, _uv, abgr });
        },
        [&](size_t sizeHint){ vertices.reserve(sizeHint); }
    };

    float minHeight = getLowerExtrudeMeters(extrude, _props) * m_tileUnitsPerMeter;
    float height = getUpperExtrudeMeters(extrude, _props) * m_tileUnitsPerMeter;

    if (minHeight != height) {

        Builders::buildPolygonExtrusion(_polygon, minHeight, height, builder);
        m_mesh->addVertices(std::move(vertices), std::move(builder.indices));

        // TODO add builder.clear() ?;
        builder.numVertices = 0;
    }

    Builders::buildPolygon(_polygon, height, builder);
    m_mesh->addVertices(std::move(vertices), std::move(builder.indices));
}

}

std::unique_ptr<StyleBuilder> PolygonStyle::createBuilder() const {
    return std::make_unique<Builder>(*this);
}

}
