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

struct Mesh : public VboMesh {

    Mesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
        : VboMesh(_vertexLayout, _drawMode) {}

    void compile(const std::vector<std::pair<uint32_t, uint32_t>>& _offsets,
                 const std::vector<PolygonVertex>& _vertices,
                 const std::vector<uint16_t>& _indices) {

        m_vertexOffsets = _offsets;

        for (auto& p : m_vertexOffsets) {
            m_nVertices += p.second;
            m_nIndices += p.first;
        }

        int stride = m_vertexLayout->getStride();
        m_glVertexData = new GLbyte[m_nVertices * stride];
        std::memcpy(m_glVertexData,
                    reinterpret_cast<const GLbyte*>(_vertices.data()),
                    m_nVertices * stride);

        m_glIndexData = new GLushort[m_nIndices];
        std::memcpy(m_glIndexData,
                    reinterpret_cast<const GLbyte*>(_indices.data()),
                    m_nIndices * sizeof(GLushort));

        m_isCompiled = true;
    }

    void compileVertexBuffer() override {}

};



PolygonStyle::PolygonStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode) {}

void PolygonStyle::constructVertexLayout() {

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

    struct {
        uint32_t order = 0;
        uint32_t color = 0xff00ffff;
        glm::vec2 extrude;
    } m_params;

    void begin(const Tile& _tile) override {
        m_tileUnitsPerMeter = _tile.getInverseScale();
        m_zoom = _tile.getID().z;
        m_mesh = std::make_unique<Mesh>(m_style.vertexLayout(), m_style.drawMode());

        m_vertexOffsets.clear();
        m_vertexOffsets.emplace_back(0,0);
        m_indices.clear();
        m_vertices.clear();
    }

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;

    const Style& style() const override { return m_style; }

    std::unique_ptr<VboMesh> build() override;

    Builder(const PolygonStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    void parseRule(const DrawRule& _rule);

    PolygonBuilder m_builder = {
        [&](const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv){
            m_vertices.push_back({ coord, m_params.order, normal, uv, m_params.color });

        },
        [&](size_t sizeHint) {}
    };

    // Used in draw for legth and offsets: sumIndices, sumVertices
    std::vector<std::pair<uint32_t, uint32_t>> m_vertexOffsets;
    std::vector<PolygonVertex> m_vertices;
    std::vector<uint16_t> m_indices;
};

std::unique_ptr<VboMesh> Builder::build() {
    auto mesh = std::make_unique<Mesh>(m_style.vertexLayout(), m_style.drawMode());
    mesh->compile(m_vertexOffsets, m_vertices, m_indices);
    return std::move(mesh);
}


void Builder::parseRule(const DrawRule& _rule) {
    _rule.get(StyleParamKey::color, m_params.color);
    _rule.get(StyleParamKey::extrude, m_params.extrude);
    _rule.get(StyleParamKey::order, m_params.order);
}

void Builder::addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) {

    parseRule(_rule);

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        m_params.color <<= (m_zoom % 6);
    }

    auto& extrude = m_params.extrude;
    float minHeight = getLowerExtrudeMeters(extrude, _props) * m_tileUnitsPerMeter;
    float height = getUpperExtrudeMeters(extrude, _props) * m_tileUnitsPerMeter;

    auto addVertices = [&](){
        auto sumVertices = m_vertexOffsets.back().second;

        if (sumVertices + m_builder.numVertices > MAX_INDEX_VALUE) {
            m_vertexOffsets.emplace_back(0, 0);
            sumVertices = 0;
            LOGE("LARGE BUFFER");
        }

        for (uint16_t idx : m_builder.indices) {
            m_indices.push_back(idx + sumVertices);
        }

        auto& vertexOffset = m_vertexOffsets.back();
        vertexOffset.first += m_builder.indices.size();
        vertexOffset.second += m_builder.numVertices;
        //LOGE("add %d %d", m_builder.indices.size(), m_builder.numVertices);

        m_builder.clear();
    };

    if (minHeight != height) {
        Builders::buildPolygonExtrusion(_polygon, minHeight, height, m_builder);
        addVertices();
    }

    Builders::buildPolygon(_polygon, height, m_builder);
    addVertices();
}

}

std::unique_ptr<StyleBuilder> PolygonStyle::createBuilder() const {
    return std::make_unique<Builder>(*this);
}

}
