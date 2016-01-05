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

    void compile(std::vector<std::pair<uint32_t, uint32_t>> _offsets,
                 std::vector<PolygonVertex> _vertices,
                 std::vector<uint16_t> _indices) {

        m_vertexOffsets.insert(m_vertexOffsets.begin(),
                               _offsets.begin(),
                               _offsets.end());

        for (auto& p : m_vertexOffsets) {
            m_nVertices += p.second;
            m_nIndices += p.first;
        }

        int stride = m_vertexLayout->getStride();
        m_glVertexData = new GLbyte[m_nVertices * stride];
        std::memcpy(m_glVertexData, (GLbyte*)_vertices.data(), m_nVertices * stride);

        m_glIndexData = new GLushort[m_nIndices];
        std::memcpy(m_glIndexData, (GLbyte*)_indices.data(), m_nIndices * sizeof(GLushort));

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

    struct Parameters {
        uint32_t order = 0;
        uint32_t color = 0xff00ffff;
        glm::vec2 extrude;
    };

    virtual void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;

    Parameters parseRule(const DrawRule& _rule) const;

    //virtual void initMesh() override { m_mesh = std::make_unique<Mesh>(m_vertexLayout, m_drawMode); }
    virtual void initMesh() override {
        m_vertexOffsets.clear();
        m_vertexOffsets.emplace_back(0,0);
        m_indices.clear();
        m_vertices.clear();
    }

    //virtual std::unique_ptr<VboMesh> build() override { return std::move(m_mesh); };
    virtual std::unique_ptr<VboMesh> build() override;

    Builder(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
        : StyleBuilder(_vertexLayout, _drawMode) {}

    PolygonBuilder m_builder = {
        [&](const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv){
            //m_vertices.push_back({ coord, normal, uv, abgr, layer });
        },
        [&](size_t sizeHint) {
            //m_vertices.reserve(m_vertices.size() + sizeHint);
        }
    };

    // Used in draw for legth and offsets: sumIndices, sumVertices
    std::vector<std::pair<uint32_t, uint32_t>> m_vertexOffsets;
    std::vector<PolygonVertex> m_vertices;
    std::vector<uint16_t> m_indices;
};

std::unique_ptr<VboMesh> Builder::build() {
    auto mesh = std::make_unique<Mesh>(m_vertexLayout, m_drawMode);
    mesh->compile(m_vertexOffsets, m_vertices, m_indices);
    return std::move(mesh);
}

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
        abgr = abgr << (m_tile->getID().z % 6);
    }

    m_builder.addVertex = [&](const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv){
        m_vertices.push_back({ coord, params.order, normal, uv, abgr });
    };

    float tileUnitsPerMeter = m_tile->getInverseScale();
    float minHeight = getLowerExtrudeMeters(extrude, _props) * tileUnitsPerMeter;
    float height = getUpperExtrudeMeters(extrude, _props) * tileUnitsPerMeter;

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
    return std::make_unique<Builder>(m_vertexLayout, m_drawMode);
}

}
