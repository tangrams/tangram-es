#include "polygonStyle.h"

#include "gl/mesh.h"
#include "gl/shaderProgram.h"
#include "marker/marker.h"
#include "material.h"
#include "scene/drawRule.h"
#include "scene/lights.h"
#include "tile/tile.h"
#include "util/builders.h"
#include "util/extrude.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/gtc/type_precision.hpp"

#include <cmath>

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

static const char* s_uniforms = R"(
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat3 u_normal_matrix;
uniform vec4 u_tile_origin;
uniform vec3 u_map_position;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;
uniform float u_proxy_depth;
uniform mat3 u_inverse_normal_matrix;)";

static const char* s_varyings = R"(
varying vec4 v_world_position;
varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;)";

void PolygonStyle::buildVertexShaderSource(ShaderSource& out, bool _selectionPass) {

    out << "#define TANGRAM_DEPTH_DELTA 0.000030518"; // (2.0 / (1 << 16)
    out << "#define TANGRAM_WORLD_POSITION_WRAP 100000.";
    out << "#define UNPACK_POSITION(x) (x / " + std::to_string(position_scale) + ")";

    insertShaderBlock("uniforms", out);
    out << s_uniforms;

    out << "attribute vec4 a_position;"
        << "attribute vec4 a_color;"
        << "attribute vec3 a_normal;";

    if (m_texCoordsGeneration) {
        out << "attribute vec2 a_texcoord;"
            << "varying vec2 v_texcoord;";
    }
    if (_selectionPass) {
        out << "attribute vec4 a_selection_color;"
            << "varying vec4 v_selection_color;";
    }
    out << s_varyings;

    out << "vec4 modelPosition() {"
        << "   return vec4(UNPACK_POSITION(a_position.xyz) * exp2(u_tile_origin.z - u_tile_origin.w), 1.0);"
        << "}"
        << "vec4 worldPosition() { return v_world_position; }"
        << "vec3 worldNormal() { return a_normal; }"
        << "vec4 modelPositionBaseZoom() { return vec4(UNPACK_POSITION(a_position.xyz), 1.0); }";

    buildMaterialAndLightGlobal(false, out);
    insertShaderBlock("global", out);

    out << "void main() {";
    out << "    vec4 position = vec4(UNPACK_POSITION(a_position.xyz), 1.0);";
    if (_selectionPass) {
        out << "    v_selection_color = a_selection_color;"
            // Skip non-selectable meshes
            << "    if (v_selection_color == vec4(0.0)) {"
            << "        gl_Position = vec4(0.0);"
            << "        return;"
            << "    }";
    } else {
        // Initialize globals
        insertShaderBlock("setup", out);
    }
    out << "    v_color = a_color;";
    if (m_texCoordsGeneration) {
        out << "    v_texcoord = a_texcoord;";
    }
    out << "    v_normal = normalize(u_normal_matrix * a_normal);";
    // Transform position into meters relative to map center
    out << "    position = u_model * position;";
    // World coordinates for 3d procedural textures
    out << "    vec4 local_origin = vec4(u_map_position.xy, 0., 0.);";
    out << "    local_origin = mod(local_origin, TANGRAM_WORLD_POSITION_WRAP);";
    out << "    v_world_position = position + local_origin;";
    // Modify position before lighting and camera projection
    insertShaderBlock("position", out);
    // Set position varying to the camera-space vertex position
    out << "    v_position = u_view * position;";

    if (!_selectionPass) {
        buildMaterialAndLightBlock(false, out);
    }
    out << "    gl_Position = u_proj * v_position;";
    // Proxy tiles are placed deeper in the depth buffer than non-proxy tiles
    out << "    gl_Position.z += TANGRAM_DEPTH_DELTA * gl_Position.w * u_proxy_depth;"
        << "    float layer = a_position.w;"
        << "    gl_Position.z -= layer * TANGRAM_DEPTH_DELTA * gl_Position.w;";
    out << "}";
}

void PolygonStyle::buildFragmentShaderSource(ShaderSource& out) {

    out << s_uniforms;
    insertShaderBlock("uniforms", out);

    out << s_varyings;

    if (m_texCoordsGeneration) {
        out << "varying vec2 v_texcoord;";
    }

    out << "vec4 worldPosition() { return v_world_position; }"
        << "vec3 worldNormal() { return normalize(u_inverse_normal_matrix * v_normal); }";

    buildMaterialAndLightGlobal(true, out);
    insertShaderBlock("global", out);

    out << "void main() {"
        << "    vec4 color = v_color;"
        << "    vec3 normal = v_normal;";

    // Initialize globals
    insertShaderBlock("setup", out);

    buildMaterialAndLightBlock(true, out);

    // Modify color after lighting (filter-like effects that don't require
    // a additional render passes)
    insertShaderBlock("filter", out);
    //color.rgb = pow(color.rgb, vec3(1.0/2.2)); // gamma correction
    out << "    gl_FragColor = color;";
    out << "}";
}


}
