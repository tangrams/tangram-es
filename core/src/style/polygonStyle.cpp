#include "polygonStyle.h"

#include "tangram.h"
#include "platform.h"
#include "material.h"
#include "util/builders.h"
#include "util/extrude.h"
#include "gl/shaderProgram.h"
#include "gl/mesh.h"
#include "tile/tile.h"
#include "scene/drawRule.h"

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/gtc/type_precision.hpp"

#include <cmath>

constexpr float position_scale = 8192.0f;
constexpr float texture_scale = 65535.0f;
constexpr float normal_scale = 127.0f;

namespace Tangram {

class PolygonMeshBase {
    using AddVertexFunc = std::function<void(const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv)>;

protected:
    GLenum m_drawMode;

public:
    PolygonMeshBase(GLenum drawMode) : m_drawMode(drawMode) {}

    struct {
        uint32_t order = 0;
        uint32_t color = 0xff00ffff;
    } attributes;

    AddVertexFunc addVertex;
    virtual std::unique_ptr<StyledMesh> build() = 0;
    virtual void clear() = 0;
    virtual std::vector<uint16_t>& indices() = 0;
    virtual std::vector<std::pair<uint32_t, uint32_t>>& offsets() = 0;
};

class PolygonMeshNoUVs : public PolygonMeshBase {
    public:
        PolygonMeshNoUVs(GLenum drawMode) : PolygonMeshBase(drawMode) {
            addVertex = [this](const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv) {
                m_meshData.vertices.push_back({ coord, attributes.order, normal, uv, attributes.color });
            };
        }

        static std::shared_ptr<VertexLayout> layout;

        struct PolygonVertexNoUVs {
            PolygonVertexNoUVs(glm::vec3 position, uint32_t order, glm::vec3 normal, glm::vec2 uv, GLuint abgr)
                : pos(glm::i16vec4{ glm::round(position * position_scale), order }),
                  norm(normal * normal_scale),
                  abgr(abgr) {}

            glm::i16vec4 pos; // pos.w contains layer (params.order)
            glm::i8vec3 norm;
            uint8_t padding = 0;
            GLuint abgr;
        };

        void clear() override { m_meshData.clear(); }

        std::unique_ptr<StyledMesh> build() override {
            if (m_meshData.vertices.empty()) { return nullptr; }

            auto mesh = std::make_unique<Mesh<PolygonVertexNoUVs>>(layout, m_drawMode);
            mesh->compile(m_meshData);
            m_meshData.clear();

            return std::move(mesh);
        }

        std::vector<std::pair<uint32_t, uint32_t>>& offsets() override {
            return m_meshData.offsets;
        }

        std::vector<uint16_t>& indices() override { return m_meshData.indices; }

    private:
        MeshData<PolygonVertexNoUVs> m_meshData;
};

std::shared_ptr<VertexLayout> PolygonMeshNoUVs::layout = std::shared_ptr<VertexLayout>(new VertexLayout({
    {"a_position", 4, GL_SHORT, false, 0},
    {"a_normal", 4, GL_BYTE, true, 0},
    {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
}));

class PolygonMesh : public PolygonMeshBase {
    public:
        PolygonMesh(GLenum drawMode) : PolygonMeshBase(drawMode) {
            addVertex = [this](const glm::vec3& coord, const glm::vec3& normal, const glm::vec2& uv) {
                m_meshData.vertices.push_back({ coord, attributes.order, normal, uv, attributes.color });
            };
        }

        static std::shared_ptr<VertexLayout> layout;
        struct PolygonVertex : PolygonMeshNoUVs::PolygonVertexNoUVs {
            PolygonVertex(glm::vec3 position, uint32_t order, glm::vec3 normal, glm::vec2 uv, GLuint abgr)
                : PolygonVertexNoUVs(position, order, normal, uv, abgr), texcoord(uv * texture_scale) {}
            glm::u16vec2 texcoord;
        };

        void clear() override { m_meshData.clear(); }

        std::unique_ptr<StyledMesh> build() override {
            if (m_meshData.vertices.empty()) { return nullptr; }

            auto mesh = std::make_unique<Mesh<PolygonVertex>>(layout, m_drawMode);
            mesh->compile(m_meshData);
            m_meshData.clear();

            return std::move(mesh);
        }

        std::vector<std::pair<uint32_t, uint32_t>>& offsets() override {
            return m_meshData.offsets;
        }

        std::vector<uint16_t>& indices() override { return m_meshData.indices; }

    private:
        MeshData<PolygonVertex> m_meshData;
};

std::shared_ptr<VertexLayout> PolygonMesh::layout = std::shared_ptr<VertexLayout>(new VertexLayout({
    {"a_position", 4, GL_SHORT, false, 0},
    {"a_normal", 4, GL_BYTE, true, 0}, // The 4th byte is for padding
    {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
    {"a_texcoord", 2, GL_UNSIGNED_SHORT, true, 0},
}));

PolygonStyle::PolygonStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode)
{}

void PolygonStyle::constructVertexLayout() {
    if (m_texCoordsGeneration) {
        m_vertexLayout = PolygonMesh::layout;
    } else {
        m_vertexLayout = PolygonMeshNoUVs::layout;
    }
}

void PolygonStyle::constructShaderProgram() {

    std::string vertShaderSrcStr = stringFromFile("shaders/polygon.vs", PathType::internal);
    std::string fragShaderSrcStr = stringFromFile("shaders/polygon.fs", PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    if (m_texCoordsGeneration) {
        m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_USE_TEX_COORDS\n");
    }
}

struct PolygonStyleBuilder : public StyleBuilder {

public:

    struct {
        glm::vec2 extrude;
        float height;
        float minHeight;
    } m_params;

    void setup(const Tile& _tile) override {
        m_tileUnitsPerMeter = _tile.getInverseScale();
        m_zoom = _tile.getID().z;
        mesh->clear();
    }

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;

    const Style& style() const override { return m_style; }

    std::unique_ptr<StyledMesh> build() override;

    PolygonStyleBuilder(const PolygonStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    void parseRule(const DrawRule& _rule, const Properties& _props);

    PolygonBuilder& polygonBuilder() { return m_builder; }

    std::unique_ptr<PolygonMeshBase> mesh;

private:

    const PolygonStyle& m_style;

    PolygonBuilder m_builder;

    float m_tileUnitsPerMeter;
    int m_zoom;

};

std::unique_ptr<StyledMesh> PolygonStyleBuilder::build() {
    return std::move(mesh->build());
}

void PolygonStyleBuilder::parseRule(const DrawRule& _rule, const Properties& _props) {
    _rule.get(StyleParamKey::color, mesh->attributes.color);
    _rule.get(StyleParamKey::extrude, m_params.extrude);
    _rule.get(StyleParamKey::order, mesh->attributes.order);

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        mesh->attributes.color <<= (m_zoom % 6);
    }

    auto& extrude = m_params.extrude;
    m_params.minHeight = getLowerExtrudeMeters(extrude, _props) * m_tileUnitsPerMeter;
    m_params.height = getUpperExtrudeMeters(extrude, _props) * m_tileUnitsPerMeter;

}

void PolygonStyleBuilder::addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) {

    parseRule(_rule, _props);

    m_builder.addVertex = mesh->addVertex;

    if (m_params.minHeight != m_params.height) {
        Builders::buildPolygonExtrusion(_polygon, m_params.minHeight, m_params.height, m_builder);
    }

    Builders::buildPolygon(_polygon, m_params.height, m_builder);

    mesh->indices().insert(mesh->indices().end(), m_builder.indices.begin(), m_builder.indices.end());

    mesh->offsets().emplace_back(m_builder.indices.size(), m_builder.numVertices);

    m_builder.clear();
}

std::unique_ptr<StyleBuilder> PolygonStyle::createBuilder() const {
    if (m_texCoordsGeneration) {
        auto builder = std::make_unique<PolygonStyleBuilder>(*this);
        builder->mesh = std::make_unique<PolygonMesh>(builder->style().drawMode());
        builder->polygonBuilder().useTexCoords = true;
        return std::move(builder);
    } else {
        auto builder = std::make_unique<PolygonStyleBuilder>(*this);
        builder->mesh = std::make_unique<PolygonMeshNoUVs>(builder->style().drawMode());
        builder->polygonBuilder().useTexCoords = false;
        return std::move(builder);
    }
}

}
