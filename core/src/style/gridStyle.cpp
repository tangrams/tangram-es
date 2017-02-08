#include "style/gridStyle.h"
#include "gl/mesh.h"
#include "gl/shaderProgram.h"
#include "glm/vec2.hpp"

#include "grid_fs.h"
#include "grid_vs.h"

namespace Tangram {

struct GridVertex {
    GridVertex(float x, float y, GLuint color) : position(x, y), color(color) {}
    glm::vec2 position;
    GLuint color;
};

GridStyle::GridStyle(std::string _name, uint32_t _resolution, Blending _blendMode, bool _selection) :
    Style(_name, _blendMode, GL_TRIANGLES, _selection), m_resolution(_resolution) {}

void GridStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
    }));
}

void GridStyle::constructShaderProgram() {
    m_shaderSource->setSourceStrings(SHADER_SOURCE(grid_fs),
                                     SHADER_SOURCE(grid_vs));
}

struct GridStyleBuilder : public StyleBuilder {
    GridStyleBuilder(const GridStyle& style) : m_style(style) {}
    void setup(const Tile& _tile) override {}
    void setup(const Marker& _marker, int zoom) override {}
    const Style& style() const override { return m_style; }
    std::unique_ptr<StyledMesh> build() override;
private:
    MeshData<GridVertex> m_meshData;
    const GridStyle& m_style;
};

std::unique_ptr<StyledMesh> GridStyleBuilder::build() {
    // Create grid vertices.
    uint32_t color = 0xffffffff;
    uint32_t resolution = m_style.resolution();
    float elementSize = 1.f / resolution;
    #if 0
    for (uint32_t col = 0; col < resolution; col++) {
        float y0 = col * elementSize;
        float y1 = (col + 1) * elementSize;
        float y2 = (col + 2) * elementSize;
        float x = 0;
        for (uint32_t row = 0; row <= resolution; row++) {
            x = row * elementSize;
            m_meshData.vertices.push_back({x, y1, color});
            m_meshData.vertices.push_back({x, y0, color});
        }
        m_meshData.vertices.push_back({x, y0, color});
        m_meshData.vertices.push_back({0, y2, color});
    }
    #else
    uint16_t index = 0;
    for (uint32_t col = 0; col <= resolution; col++) {
        float y = col * elementSize;
        for (uint32_t row = 0; row <= resolution; row++) {
            float x = row * elementSize;
            m_meshData.vertices.push_back({x, y, color});

            if (row < resolution && col < resolution) {
                m_meshData.indices.push_back(index);
                m_meshData.indices.push_back(index + 1);
                m_meshData.indices.push_back(index + resolution + 1);

                m_meshData.indices.push_back(index + 1);
                m_meshData.indices.push_back(index + resolution + 2);
                m_meshData.indices.push_back(index + resolution + 1);
            }
            index++;
        }
    }
    #endif
    m_meshData.offsets.emplace_back(m_meshData.indices.size(), m_meshData.vertices.size());
    // Create mesh from vertices.
    auto mesh = std::make_unique<Mesh<GridVertex>>(m_style.vertexLayout(), m_style.drawMode());
    mesh->compile(m_meshData);
    m_meshData.clear();
    return std::move(mesh);
}

std::unique_ptr<StyleBuilder> GridStyle::createBuilder() const {
    auto builder = std::make_unique<GridStyleBuilder>(*this);
    return std::move(builder);
}

} // namespace Tangram
