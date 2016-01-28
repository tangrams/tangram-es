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


struct Mesh : public VboMesh {

    Mesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
        : VboMesh(_vertexLayout, _drawMode) {}

    void compile(const std::vector<std::pair<uint32_t, uint32_t>>& _offsets,
                 const std::vector<PolylineVertex>& _vertices,
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

namespace {

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

    std::unique_ptr<Mesh> m_mesh;
    float m_tileUnitsPerMeter;
    float m_tileSize;
    int m_zoom;

    // Used in draw for legth and offsets: sumIndices, sumVertices
    std::vector<std::pair<uint32_t, uint32_t>> m_vertexOffsets;
    std::vector<PolylineVertex> m_vertices;
    std::vector<uint16_t> m_indices;

    void begin(const Tile& _tile) override;

    const Style& style() const override { return m_style; }

    void addFeature(const Feature& _feat, const DrawRule& _rule) override;

    std::unique_ptr<VboMesh> build() override;

    Builder(const PolylineStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    void addMesh(const Line& _line, const Parameters& _params);

    void buildLine(const Line& _line, PolyLineBuilder& _builder);

    Parameters parseRule(const DrawRule& _rule, const Properties& _props);

    bool evalWidth(const StyleParam& _styleParam, float& width, float& slope);

};

void Builder::begin(const Tile& _tile) {
    m_tileUnitsPerMeter = _tile.getInverseScale();
    m_zoom = _tile.getID().z;
    m_tileSize = _tile.getProjection()->TileSize();
    m_mesh = std::make_unique<Mesh>(m_style.vertexLayout(), m_style.drawMode());

    m_vertexOffsets.clear();
    m_vertexOffsets.emplace_back(0,0);
    m_indices.clear();
    m_vertices.clear();
}

std::unique_ptr<VboMesh> Builder::build() {
    auto mesh = std::make_unique<Mesh>(m_style.vertexLayout(), m_style.drawMode());
    mesh->compile(m_vertexOffsets, m_vertices, m_indices);
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

void Builder::buildLine(const Line& _line, PolyLineBuilder& _builder) {

    Builders::buildPolyLine(_line, _builder);

    auto sumVertices = m_vertexOffsets.back().second;
    if (sumVertices + _builder.numVertices > MAX_INDEX_VALUE) {
        m_vertexOffsets.emplace_back(0, 0);
        // Indices must reference vertices starting at 0
        sumVertices = 0;
    }
    for (uint16_t idx : _builder.indices) {
        m_indices.push_back(idx + sumVertices);
    }
    auto& vertexOffset = m_vertexOffsets.back();
    vertexOffset.first += _builder.indices.size();
    vertexOffset.second += _builder.numVertices;

    _builder.clear();
}

void Builder::addMesh(const Line& _line, const Parameters& params) {

    GLuint abgr = params.fill.color;

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr <<= (m_zoom % 6);
    }

    // Keep current vertex buffer position when the
    // same vertices are used for the outline.
    size_t startIndex = m_indices.size();
    size_t startVertex = m_vertices.size();

    float order = params.fill.order;
    float width = params.fill.width;
    float slope = params.fill.slope;

    struct {
        glm::i16vec2 height;
        glm::i16vec2 width;
        GLuint color;
        void set(uint32_t _color, float _width, float _dWdZ, float _height, float _order) {
            height = { glm::round(_height * position_scale), _order * order_scale};
            width = glm::vec2{_width, _dWdZ} * extrusion_scale;
            color =_color;
        }
    } options;

    PolyLineBuilder builder = {
        [this, &p = options](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& uv) {
            m_vertices.push_back({{coord.x,coord.y}, normal, uv, p.width, p.height, p.color});
        },
        [&](size_t sizeHint) {}
    };

    builder.cap = params.fill.cap;
    builder.join = params.fill.join;
    builder.keepTileEdges = params.keepTileEdges;
    options.set(abgr, width, slope, params.height, order);

    buildLine(_line, builder);

    if (!params.outlineOn) { return; }

    width = params.outline.width;
    slope = params.outline.slope;
    abgr = params.outline.color;
    order = std::min(params.outline.order, params.fill.order) - .5f;

    if (params.outline.cap != params.fill.cap || params.outline.join != params.fill.join) {
        // need to re-triangulate with different cap and/or join
        builder.cap = params.outline.cap;
        builder.join = params.outline.join;
        options.set(abgr, width, slope, params.height, order);

        buildLine(_line, builder);

    } else {
        // re-use indices from original line

        // TODO: This is a bit too much shuffling with offsets
        size_t numIndices = m_indices.size() - startIndex;
        size_t numVertices = m_vertices.size() - startVertex;
        int shift = 0;

        auto curVertices = m_vertexOffsets.back().second;
        if (curVertices + numVertices > MAX_INDEX_VALUE) {
            m_vertexOffsets.emplace_back(0, 0);
            // Indices must reference vertices starting at 0
            shift = - (m_vertices.size() - numVertices);
        } else {
            // Shift indices to start at the copied vertices
            shift = numVertices;
        }

        for (size_t i = 0; i < numIndices; i++) {
            m_indices.push_back(m_indices[startIndex + i] + shift);
        }

        auto& vertexOffset = m_vertexOffsets.back();
        vertexOffset.first += numIndices;
        vertexOffset.second += numVertices;

        for (size_t i = 0; i < numVertices; i++) {
            const auto& v = m_vertices[startVertex + i];
            m_vertices.push_back({ v, order, {width, slope}, abgr });
        }
    }
}

}

std::unique_ptr<StyleBuilder> PolylineStyle::createBuilder() const {
    return std::make_unique<Builder>(*this);
}

}
