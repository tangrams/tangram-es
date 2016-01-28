#include "polylineStyle.h"

#include "tangram.h"
#include "platform.h"
#include "material.h"
#include "gl/shaderProgram.h"
#include "gl/typedMesh.h"
#include "scene/stops.h"
#include "scene/drawRule.h"
#include "tile/tile.h"
#include "util/builders.h"
#include "util/mapProjection.h"
#include "util/extrude.h"

#include "glm/vec3.hpp"
#include "glm/gtc/type_precision.hpp"

constexpr float extrusion_scale = 4096.0f;
constexpr float position_scale = 8192.0f;
constexpr float texture_scale = 65535.0f;
constexpr float order_scale = 2.0f;

namespace Tangram {

struct PolylineVertex {
    PolylineVertex(glm::vec2 position, glm::vec2 extrude, glm::vec2 uv,
                   glm::i16vec2 width, glm::i16vec2 height, GLuint abgr)

        : pos(glm::i16vec2{ glm::round(position * position_scale)}, height),
          texcoord(uv * texture_scale),
          extrude(glm::i16vec2{extrude * extrusion_scale}, width),
          abgr(abgr) {}

    PolylineVertex(PolylineVertex v, float order, glm::vec2 width, GLuint abgr)
        : pos(glm::i16vec4{glm::i16vec3{v.pos}, order * order_scale}),
          texcoord(v.texcoord),
          extrude(glm::i16vec4{ v.extrude.x, v.extrude.y, width * extrusion_scale }),
          abgr(abgr) {}

    glm::i16vec4 pos;
    glm::u16vec2 texcoord;
    glm::i16vec4 extrude;
    GLuint abgr;
};

PolylineStyle::PolylineStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode) {
}

void PolylineStyle::constructVertexLayout() {

    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 4, GL_SHORT, false, 0},
        {"a_texcoord", 2, GL_UNSIGNED_SHORT, true, 0},
        {"a_extrude", 4, GL_SHORT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
    }));
}

void PolylineStyle::constructShaderProgram() {

    std::string vertShaderSrcStr = stringFromFile("shaders/polyline.vs", PathType::internal);
    std::string fragShaderSrcStr = stringFromFile("shaders/polyline.fs", PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

namespace { // Builder

struct MeshData {
    std::vector<std::pair<uint32_t, uint32_t>> offsets;
    std::vector<PolylineVertex> vertices;
    std::vector<uint16_t> indices;

    // Temporary values for the currently build mesh
    glm::i16vec2 height;
    glm::i16vec2 width;
    GLuint color;
    void set(uint32_t _color, float _width, float _dWdZ, float _height, float _order) {
        height = { glm::round(_height * position_scale), _order * order_scale};
        width = glm::vec2{_width, _dWdZ} * extrusion_scale;
        color =_color;
    }

    void clear() {
        offsets.clear();
        indices.clear();
        vertices.clear();
    }
};

struct Mesh : public VboMesh {

    Mesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
        : VboMesh(_vertexLayout, _drawMode) {}

    // Add indices by collecting them into batches to draw as much as
    // possible in one draw call.  The indices must be shifted by the
    // number of vertices that are present in the current batch.
    GLushort* compileIndices(GLushort* dst, const MeshData& data) {
        m_vertexOffsets.emplace_back(0, 0);
        size_t curVertices = 0;
        size_t src = 0;

        for (auto& p : data.offsets) {
            size_t nIndices = p.first;
            size_t nVertices = p.second;

            if (curVertices + nVertices > MAX_INDEX_VALUE) {
                m_vertexOffsets.emplace_back(0, 0);
                curVertices = 0;
            }
            for (size_t i = 0; i < nIndices; i++, dst++) {
                *dst = data.indices[src++] + curVertices;
            }
            auto& offset = m_vertexOffsets.back();
            offset.first += nIndices;
            offset.second += nVertices;

            curVertices += nVertices;
        }
        return dst;
    }

    // Use custom 'compile' to directly transfer buffers from MeshData
    void compile(const MeshData& _fill, const MeshData& _outline) {
        m_isCompiled = true;

        size_t nFillVertices = 0;
        size_t nFillIndices = 0;
        for (auto& p : _fill.offsets) {
            nFillVertices += p.second;
            nFillIndices += p.first;
        }
        size_t nOutlineVertices = 0;
        size_t nOutlineIndices = 0;
        for (auto& p : _outline.offsets) {
            nOutlineVertices += p.second;
            nOutlineIndices += p.first;
        }
        m_nVertices = nFillVertices + nOutlineVertices;
        m_nIndices = nFillIndices + nOutlineIndices;

        int stride = m_vertexLayout->getStride();
        m_glVertexData = new GLbyte[m_nVertices * stride];
        m_glIndexData = new GLushort[m_nIndices];

        std::memcpy(m_glVertexData,
                    reinterpret_cast<const GLbyte*>(_outline.vertices.data()),
                    nOutlineVertices * stride);

        std::memcpy(m_glVertexData + (nOutlineVertices * stride),
                    reinterpret_cast<const GLbyte*>(_fill.vertices.data()),
                    nFillVertices * stride);

        GLushort* dst = m_glIndexData;
        dst = compileIndices(dst, _outline);
        dst = compileIndices(dst, _fill);

        assert(dst == m_glIndexData + m_nIndices);
    }

    void compileVertexBuffer() override {}
};

struct Parameters {

    struct {
        uint32_t order = 0;
        uint32_t color = 0xff00ffff;
        float width = 0.f;
        float slope = 0.f;
        CapTypes cap = CapTypes::butt;
        JoinTypes join = JoinTypes::miter;
    } fill, outline;

    float height = 0.f;
    bool keepTileEdges = false;
    bool outlineOn = false;
};

struct Builder : public StyleBuilder {

    const PolylineStyle& m_style;
    PolyLineBuilder m_builder;
    MeshData m_fill, m_outline;

    std::unique_ptr<Mesh> m_mesh;
    float m_tileUnitsPerMeter;
    float m_tileSize;
    int m_zoom;

    void begin(const Tile& _tile) override;

    const Style& style() const override { return m_style; }

    void addFeature(const Feature& _feat, const DrawRule& _rule) override;

    std::unique_ptr<VboMesh> build() override;

    Builder(const PolylineStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    void addMesh(const Line& _line, const Parameters& _params);

    void buildLine(const Line& _line, MeshData& _mesh);

    Parameters parseRule(const DrawRule& _rule, const Properties& _props);

    bool evalWidth(const StyleParam& _styleParam, float& width, float& slope);

};

void Builder::begin(const Tile& _tile) {
    m_tileUnitsPerMeter = _tile.getInverseScale();
    m_zoom = _tile.getID().z;
    m_tileSize = _tile.getProjection()->TileSize();
    m_mesh = std::make_unique<Mesh>(m_style.vertexLayout(), m_style.drawMode());
    m_fill.clear();
    m_outline.clear();
}

std::unique_ptr<VboMesh> Builder::build() {
    auto mesh = std::make_unique<Mesh>(m_style.vertexLayout(), m_style.drawMode());
    mesh->compile(m_fill, m_outline);
    m_fill.clear();
    m_outline.clear();
    return std::move(mesh);
}

auto Builder::parseRule(const DrawRule& _rule, const Properties& _props) -> Parameters {
    Parameters p;

    uint32_t cap = 0, join = 0;

    auto& width = _rule.findParameter(StyleParamKey::width);
    if (!evalWidth(width, p.fill.width, p.fill.slope)) {
        p.fill.width = 0;
        return p;
    }
    p.fill.slope -= p.fill.width;
    _rule.get(StyleParamKey::color, p.fill.color);
    _rule.get(StyleParamKey::cap, cap);
    _rule.get(StyleParamKey::join, join);
    _rule.get(StyleParamKey::order, p.fill.order);
    _rule.get(StyleParamKey::tile_edges, p.keepTileEdges);

    p.fill.cap = static_cast<CapTypes>(cap);
    p.fill.join = static_cast<JoinTypes>(join);

    glm::vec2 extrude = glm::vec2(0);
    _rule.get(StyleParamKey::extrude, extrude);

    p.height = getUpperExtrudeMeters(extrude, _props);
    p.height *= m_tileUnitsPerMeter;

    p.outline.order = p.fill.order;
    p.outline.cap = p.fill.cap;
    p.outline.join = p.fill.join;

    auto& outlineWidth = _rule.findParameter(StyleParamKey::outline_width);
    if (outlineWidth |
        _rule.get(StyleParamKey::outline_color, p.outline.color) |
        _rule.get(StyleParamKey::outline_order, p.outline.order) |
        _rule.get(StyleParamKey::outline_cap, cap) |
        _rule.get(StyleParamKey::outline_join, join)) {

        p.outline.cap = static_cast<CapTypes>(cap);
        p.outline.join = static_cast<JoinTypes>(join);

        if (!evalWidth(outlineWidth, p.outline.width, p.outline.slope)) {
            return p;
        }

        // NB: Multiply by 2 for the outline to get the expected stroke pixel width.
        p.outline.width *= 2.0f;
        p.outline.slope *= 2.0f;
        p.outline.slope -= p.outline.width;

        p.outline.width += p.fill.width;
        p.outline.slope += p.fill.slope;

        p.outlineOn = true;
    }

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        p.fill.color <<= (m_zoom % 6);
        p.outline.color <<= (m_zoom % 6);
    }

    // FIXME: just to make hidden lines visible
    if (p.fill.width < 0.5/m_tileSize) { p.fill.width = 0.5/m_tileSize; }

    return p;
}

double widthMeterToPixel(int _zoom, double _tileSize, double _width) {
    // pixel per meter at z == 0
    double meterRes = _tileSize / (2.0 * MapProjection::HALF_CIRCUMFERENCE);
    // pixel per meter at zoom
    meterRes *= exp2(_zoom);

    return _width * meterRes;
}

bool Builder::evalWidth(const StyleParam& _styleParam, float& width, float& slope) {

    // NB: 0.5 because 'width' will be extruded in both directions
    float tileRes = 0.5 / m_tileSize;

    // auto& styleParam = _rule.findParameter(_key);
    if (_styleParam.stops) {

        width = _styleParam.value.get<float>();
        width *= tileRes;

        slope = _styleParam.stops->evalWidth(m_zoom + 1);
        slope *= tileRes;
        return true;
    }

    if (_styleParam.value.is<StyleParam::Width>()) {
        auto& widthParam = _styleParam.value.get<StyleParam::Width>();

        width = widthParam.value;

        if (widthParam.isMeter()) {
            width = widthMeterToPixel(m_zoom, m_tileSize, width);
            width *= tileRes;
            slope = width * 2;
        } else {
            width *= tileRes;
            slope = width;
        }
        return true;
    }

    LOGD("Invalid type for Width '%d'", _styleParam.value.which());
    return false;
}

void Builder::addFeature(const Feature& _feat, const DrawRule& _rule) {

    if (_feat.geometryType == GeometryType::points) { return; }
    if (!checkRule(_rule)) { return; }

    Parameters params = parseRule(_rule, _feat.props);

    if (params.fill.width <= 0.0f && params.fill.slope <= 0.0f ) { return; }

    if (_feat.geometryType == GeometryType::lines) {
        // Line geometries are never clipped to tiles, so keep all segments
        params.keepTileEdges = true;

        for (auto& line : _feat.lines) {
            addMesh(line, params);
        }
    } else {
        for (auto& polygon : _feat.polygons) {
            for (const auto& line : polygon) {
                addMesh(line, params);
            }
        }
    }
}

void Builder::buildLine(const Line& _line, MeshData& _mesh) {

    m_builder.addVertex = [&_mesh](const glm::vec3& coord,
                                   const glm::vec2& normal,
                                   const glm::vec2& uv) {
        _mesh.vertices.push_back({{ coord.x,coord.y }, normal, uv,
                              _mesh.width, _mesh.height, _mesh.color});
    };

    Builders::buildPolyLine(_line, m_builder);

    _mesh.indices.insert(_mesh.indices.end(),
                         m_builder.indices.begin(),
                         m_builder.indices.end());

    _mesh.offsets.emplace_back(m_builder.indices.size(),
                               m_builder.numVertices);

    m_builder.clear();
}

void Builder::addMesh(const Line& _line, const Parameters& _params) {

    m_builder.cap = _params.fill.cap;
    m_builder.join = _params.fill.join;
    m_builder.keepTileEdges = _params.keepTileEdges;
    m_fill.set(_params.fill.color,
               _params.fill.width,
               _params.fill.slope,
               _params.height,
               _params.fill.order);

    buildLine(_line, m_fill);

    if (!_params.outlineOn) { return; }

    float order = std::min(_params.outline.order, _params.fill.order) - .5f;

    if (_params.outline.cap != _params.fill.cap ||
        _params.outline.join != _params.fill.join) {
        // need to re-triangulate with different cap and/or join
        m_builder.cap = _params.outline.cap;
        m_builder.join = _params.outline.join;
        m_outline.set(_params.outline.color,
                      _params.outline.width,
                      _params.outline.slope,
                      _params.height, order);

        buildLine(_line, m_outline);

    } else {
        // reuse indices from original line, overriding color and width
        size_t nIndices = m_fill.offsets.back().first;
        size_t nVertices = m_fill.offsets.back().second;
        m_outline.offsets.emplace_back(nIndices, nVertices);

        auto indicesIt = m_fill.indices.end() - nIndices;
        m_outline.indices.insert(m_outline.indices.end(),
                                 indicesIt,
                                 m_fill.indices.end());

        auto vertexIt = m_fill.vertices.end() - nVertices;

        glm::vec2 width = {_params.outline.width, _params.outline.slope };
        GLuint abgr = _params.outline.color;

        for (; vertexIt != m_fill.vertices.end(); ++vertexIt) {
            m_outline.vertices.emplace_back(*vertexIt, order, width, abgr);
        }
    }
}

}

std::unique_ptr<StyleBuilder> PolylineStyle::createBuilder() const {
    return std::make_unique<Builder>(*this);
}

}
