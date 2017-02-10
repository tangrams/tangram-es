#include "style/polygonStyle.h"

#include "gl/mesh.h"
#include "gl/shaderProgram.h"
#include "marker/marker.h"
#include "material.h"
#include "platform.h"
#include "scene/drawRule.h"
#include "tangram.h"
#include "tile/tile.h"
#include "util/builders.h"
#include "util/extrude.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/gtc/type_precision.hpp"
#include <cmath>

#include "polygon_fs.h"
#include "polygon_vs.h"

constexpr float position_scale = 8192.0f;
constexpr float texture_scale = 65535.0f;
constexpr float normal_scale = 127.0f;

namespace Tangram {


struct PolygonVertexNoUVs {

    PolygonVertexNoUVs(glm::vec3 position, uint32_t order, glm::vec3 normal, glm::vec2 uv, GLuint abgr, GLuint selection)
        : pos(glm::i16vec4{ glm::round(position * position_scale), order }),
          norm(normal * normal_scale),
          abgr(abgr),
          selection(selection) {}

    glm::i16vec4 pos; // pos.w contains layer (params.order)
    glm::i8vec3 norm;
    uint8_t padding = 0;
    GLuint abgr;
    GLuint selection;
};

struct PolygonVertex : PolygonVertexNoUVs {

    PolygonVertex(glm::vec3 position, uint32_t order, glm::vec3 normal, glm::vec2 uv, GLuint abgr, GLuint selection)
        : PolygonVertexNoUVs(position, order, normal, uv, abgr, selection), texcoord(uv * texture_scale) {}

    glm::u16vec2 texcoord;
};

PolygonStyle::PolygonStyle(std::string _name, Blending _blendMode, GLenum _drawMode, bool _selection)
    : Style(_name, _blendMode, _drawMode, _selection)
{}

void PolygonStyle::constructVertexLayout() {

    if (m_texCoordsGeneration) {
        m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 4, GL_SHORT, false, 0},
            {"a_normal", 4, GL_BYTE, true, 0}, // The 4th byte is for padding
            {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
            {"a_selection_color", 4, GL_UNSIGNED_BYTE, true, 0},
            {"a_texcoord", 2, GL_UNSIGNED_SHORT, true, 0},
        }));
    } else {
        m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 4, GL_SHORT, false, 0},
            {"a_normal", 4, GL_BYTE, true, 0},
            {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
            {"a_selection_color", 4, GL_UNSIGNED_BYTE, true, 0},
        }));
    }

}

void PolygonStyle::constructShaderProgram() {

    m_shaderSource->setSourceStrings(SHADER_SOURCE(polygon_fs),
                                      SHADER_SOURCE(polygon_vs));

    if (m_texCoordsGeneration) {
        m_shaderSource->addSourceBlock("defines", "#define TANGRAM_USE_TEX_COORDS\n");
    }
}

template <class V>
struct PolygonStyleBuilder : public StyleBuilder {

public:

    struct {
        uint32_t order = 0;
        uint32_t color = 0xffffffff;
        glm::vec2 extrude;
        float height;
        float minHeight;
        uint32_t selectionColor = 0;
    } m_params;

    void setup(const Tile& _tile) override {
        m_tileUnitsPerMeter = _tile.getInverseScale();
        m_zoom = _tile.getID().z;
        m_meshData.clear();
    }

    void setup(const Marker& _marker, int zoom) override {
        m_zoom = zoom;
        m_tileUnitsPerMeter = 1.f / _marker.extent();
        m_meshData.clear();
    }

    bool addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;

    const Style& style() const override { return m_style; }

    std::unique_ptr<StyledMesh> build() override;

    PolygonStyleBuilder(const PolygonStyle& _style) : m_style(_style) {}

    void parseRule(const DrawRule& _rule, const Properties& _props);

    PolygonBuilder& polygonBuilder() { return m_builder; }

private:

    const PolygonStyle& m_style;

    PolygonBuilder m_builder;

    MeshData<V> m_meshData;

    float m_tileUnitsPerMeter = 0;
    int m_zoom = 0;

};

template <class V>
std::unique_ptr<StyledMesh> PolygonStyleBuilder<V>::build() {
    if (m_meshData.vertices.empty()) { return nullptr; }

    auto mesh = std::make_unique<Mesh<V>>(m_style.vertexLayout(),
                                                      m_style.drawMode());
    mesh->compile(m_meshData);
    m_meshData.clear();

    return std::move(mesh);
}

template <class V>
void PolygonStyleBuilder<V>::parseRule(const DrawRule& _rule, const Properties& _props) {
    _rule.get(StyleParamKey::color, m_params.color);
    _rule.get(StyleParamKey::extrude, m_params.extrude);
    _rule.get(StyleParamKey::order, m_params.order);

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        m_params.color <<= (m_zoom % 6);
    }

    auto& extrude = m_params.extrude;
    m_params.minHeight = getLowerExtrudeMeters(extrude, _props) * m_tileUnitsPerMeter;
    m_params.height = getUpperExtrudeMeters(extrude, _props) * m_tileUnitsPerMeter;

    m_params.selectionColor = _rule.selectionColor;
}

template <class V>
bool PolygonStyleBuilder<V>::addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) {

    parseRule(_rule, _props);

    m_builder.addVertex = [this](const glm::vec3& coord,
                                 const glm::vec3& normal,
                                 const glm::vec2& uv) {
        m_meshData.vertices.push_back({ coord, m_params.order, normal, uv, m_params.color, m_params.selectionColor });
    };

    if (m_params.minHeight != m_params.height) {
        Builders::buildPolygonExtrusion(_polygon, m_params.minHeight,
                                        m_params.height, m_builder);
    }

    Builders::buildPolygon(_polygon, m_params.height, m_builder);

    m_meshData.indices.insert(m_meshData.indices.end(),
                              m_builder.indices.begin(),
                              m_builder.indices.end());

    m_meshData.offsets.emplace_back(m_builder.indices.size(),
                                    m_builder.numVertices);
    m_builder.clear();

    return true;
}

std::unique_ptr<StyleBuilder> PolygonStyle::createBuilder() const {
    if (m_texCoordsGeneration) {
        auto builder = std::make_unique<PolygonStyleBuilder<PolygonVertex>>(*this);
        builder->polygonBuilder().useTexCoords = true;
        return std::move(builder);
    } else {
        auto builder = std::make_unique<PolygonStyleBuilder<PolygonVertexNoUVs>>(*this);
        builder->polygonBuilder().useTexCoords = false;
        return std::move(builder);
    }
}

}
