#include "polylineStyle.h"

#include "tangram.h"
#include "platform.h"
#include "material.h"
#include "gl/shaderProgram.h"
#include "gl/mesh.h"
#include "gl/texture.h"
#include "scene/stops.h"
#include "scene/drawRule.h"
#include "tile/tile.h"
#include "util/builders.h"
#include "util/mapProjection.h"
#include "util/extrude.h"
#include "util/dashArray.h"

#include "shaders/polyline_vs.h"
#include "shaders/polyline_fs.h"

#include "glm/vec3.hpp"
#include "glm/gtc/type_precision.hpp"

constexpr float extrusion_scale = 4096.0f;
constexpr float position_scale = 8192.0f;
constexpr float texture_scale = 8192.0f;
constexpr float order_scale = 2.0f;

namespace Tangram {

struct PolylineVertexNoUVs {
    PolylineVertexNoUVs(glm::vec2 position, glm::vec2 extrude, glm::vec2 uv,
                   glm::i16vec2 width, glm::i16vec2 height, GLuint abgr)
        : pos(glm::i16vec2{ glm::round(position * position_scale)}, height),
          extrude(glm::i16vec2{extrude * extrusion_scale}, width),
          abgr(abgr) {}

    PolylineVertexNoUVs(PolylineVertexNoUVs v, short order, glm::i16vec2 width, GLuint abgr)
        : pos(glm::i16vec4{glm::i16vec3{v.pos}, order}),
          extrude(glm::i16vec4{ v.extrude.x, v.extrude.y, width }),
          abgr(abgr) {}

    glm::i16vec4 pos;
    glm::i16vec4 extrude;
    GLuint abgr;
};

struct PolylineVertex : PolylineVertexNoUVs {
    PolylineVertex(glm::vec2 position, glm::vec2 extrude, glm::vec2 uv,
                   glm::i16vec2 width, glm::i16vec2 height, GLuint abgr)
        : PolylineVertexNoUVs(position, extrude, uv, width, height, abgr),
          texcoord(uv * texture_scale) {}

    PolylineVertex(PolylineVertex v, short order, glm::i16vec2 width, GLuint abgr)
        : PolylineVertexNoUVs(v, order, width, abgr),
          texcoord(v.texcoord) {}

    glm::u16vec2 texcoord;
};

PolylineStyle::PolylineStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode)
{}

void PolylineStyle::constructVertexLayout() {

    // TODO: Ideally this would be in the same location as the struct that it basically describes
    if (m_texCoordsGeneration) {
        m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 4, GL_SHORT, false, 0},
            {"a_extrude", 4, GL_SHORT, false, 0},
            {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
            {"a_texcoord", 2, GL_UNSIGNED_SHORT, false, 0},
        }));
    } else {
        m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 4, GL_SHORT, false, 0},
            {"a_extrude", 4, GL_SHORT, false, 0},
            {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        }));
    }

}

void PolylineStyle::onBeginDrawFrame(const View& _view, Scene& _scene) {
    if (m_texture) {
        m_texture->update(0);
    }
}

void PolylineStyle::constructShaderProgram() {

    if (m_dashArray.size() > 0) {
        TextureOptions options {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE}};
        auto pixels = DashArray::render(m_dashArray);
        m_texture = std::make_unique<Texture>(pixels.size(), 1, options);
        m_texture->setData(pixels.data(), pixels.size());
    }

    m_shaderProgram->setSourceStrings(SHADER_SOURCE(polyline_fs),
                                      SHADER_SOURCE(polyline_vs));

    if (m_texCoordsGeneration) {
        m_shaderProgram->addSourceBlock("defines", "#define TANGRAM_USE_TEX_COORDS\n");
    }
}

template <class V>
struct PolylineStyleBuilder : public StyleBuilder {

public:

    struct Parameters {

        struct Attributes {
            // Values prepared for the currently build mesh
            glm::i16vec2 height;
            glm::i16vec2 width;
            uint32_t color;
            float miterLimit = 3.0;
            CapTypes cap = CapTypes::butt;
            JoinTypes join = JoinTypes::miter;

            void set(float _width, float _dWdZ, float _height, float _order) {
                height = { glm::round(_height * position_scale), _order * order_scale};
                width = { glm::round(_width * extrusion_scale), glm::round(_dWdZ * extrusion_scale) };
            }
        } fill, stroke;

        bool keepTileEdges = false;
        bool closedPolygon = false;
        bool outlineOn = false;
        bool lineOn = true;
    };

    void setup(const Tile& _tile) override;

    const Style& style() const override { return m_style; }

    void addFeature(const Feature& _feat, const DrawRule& _rule) override;

    std::unique_ptr<StyledMesh> build() override;

    PolylineStyleBuilder(const PolylineStyle& _style)
        : StyleBuilder(_style), m_style(_style),
          m_meshData(2) {}

    void addMesh(const Line& _line, const Parameters& _params);

    void buildLine(const Line& _line, const typename Parameters::Attributes& _att,
                   MeshData<V>& _mesh);

    Parameters parseRule(const DrawRule& _rule, const Properties& _props);

    bool evalWidth(const StyleParam& _styleParam, float& width, float& slope);

    PolyLineBuilder& polylineBuilder() { return m_builder; }

private:

    const PolylineStyle& m_style;
    PolyLineBuilder m_builder;

    std::vector<MeshData<V>> m_meshData;

    float m_tileUnitsPerMeter;
    float m_tileSizePixels;
    int m_zoom;
};

template <class V>
void PolylineStyleBuilder<V>::setup(const Tile& tile) {

    const auto& id = tile.getID();

    // Use the 'style zoom' to evaluate style parameters.
    m_zoom = id.s;
    m_tileUnitsPerMeter = tile.getInverseScale();
    m_tileSizePixels = tile.getProjection()->TileSize();

    // When a tile is overzoomed, we are actually styling the area of its
    // 'source' tile, which will have a larger effective pixel size at the
    // 'style' zoom level. This scaling is performed in the vertex shader to
    // prevent loss of precision for small dimensions in packed attributes.
}

template <class V>
std::unique_ptr<StyledMesh> PolylineStyleBuilder<V>::build() {
    if (m_meshData[0].vertices.empty() &&
        m_meshData[1].vertices.empty()) {
        return nullptr;
    }

    auto mesh = std::make_unique<Mesh<V>>(m_style.vertexLayout(), m_style.drawMode());

    bool painterMode = (m_style.blendMode() == Blending::overlay ||
                        m_style.blendMode() == Blending::inlay);

    // Swap draw order to draw outline first when not using depth testing
    if (painterMode) { std::swap(m_meshData[0], m_meshData[1]); }

    mesh->compile(m_meshData);

    // Swapping back since fill mesh may have more vertices than outline
    if (painterMode) { std::swap(m_meshData[0], m_meshData[1]); }

    m_meshData[0].clear();
    m_meshData[1].clear();
    return std::move(mesh);
}

template <class V>
auto PolylineStyleBuilder<V>::parseRule(const DrawRule& _rule, const Properties& _props) -> Parameters {
    Parameters p;

    uint32_t cap = 0, join = 0;

    struct {
        uint32_t order = 0;
        uint32_t color = 0xff00ffff;
        float width = 0.f;
        float slope = 0.f;
    } fill, stroke;

    auto& width = _rule.findParameter(StyleParamKey::width);
    if (!evalWidth(width, fill.width, fill.slope)) {
        fill.width = 0;
        return p;
    }
    fill.slope -= fill.width;
    _rule.get(StyleParamKey::color, p.fill.color);
    _rule.get(StyleParamKey::cap, cap);
    _rule.get(StyleParamKey::join, join);
    _rule.get(StyleParamKey::order, fill.order);
    _rule.get(StyleParamKey::tile_edges, p.keepTileEdges);
    _rule.get(StyleParamKey::miter_limit, p.fill.miterLimit);

    p.fill.cap = static_cast<CapTypes>(cap);
    p.fill.join = static_cast<JoinTypes>(join);

    glm::vec2 extrude = glm::vec2(0);
    _rule.get(StyleParamKey::extrude, extrude);

    float height = getUpperExtrudeMeters(extrude, _props);
    height *= m_tileUnitsPerMeter;

    p.fill.set(fill.width, fill.slope, height, fill.order);
    p.lineOn = !_rule.isOutlineOnly;

    stroke.order = fill.order;
    p.stroke.cap = p.fill.cap;
    p.stroke.join = p.fill.join;
    p.stroke.miterLimit = p.fill.miterLimit;

    auto& strokeWidth = _rule.findParameter(StyleParamKey::outline_width);
    if (!p.lineOn || !_rule.findParameter(StyleParamKey::outline_style)) {
        if (strokeWidth |
            _rule.get(StyleParamKey::outline_order, stroke.order) |
            _rule.get(StyleParamKey::outline_cap, cap) |
            _rule.get(StyleParamKey::outline_join, join) |
            _rule.get(StyleParamKey::outline_miter_limit, p.stroke.miterLimit)) {

            p.stroke.cap = static_cast<CapTypes>(cap);
            p.stroke.join = static_cast<JoinTypes>(join);

            if (!_rule.get(StyleParamKey::outline_color, p.stroke.color)) { return p; }
            if (!evalWidth(strokeWidth, stroke.width, stroke.slope)) {
                return p;
            }

            // NB: Multiply by 2 for the stroke to get the expected stroke pixel width.
            stroke.width *= 2.0f;
            stroke.slope *= 2.0f;
            stroke.slope -= stroke.width;

            stroke.width += fill.width;
            stroke.slope += fill.slope;

            stroke.order = std::min(stroke.order, fill.order);

            p.stroke.set(stroke.width, stroke.slope,
                    height, stroke.order - 0.5f);

            p.outlineOn = true;
        }
    }

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        fill.color <<= (m_zoom % 6);
        stroke.color <<= (m_zoom % 6);
    }

    return p;
}

double widthMeterToPixel(int _zoom, double _tileSize, double _width) {
    // pixel per meter at z == 0
    double meterRes = _tileSize / (2.0 * MapProjection::HALF_CIRCUMFERENCE);
    // pixel per meter at zoom
    meterRes *= exp2(_zoom);

    return _width * meterRes;
}

template <class V>
bool PolylineStyleBuilder<V>::evalWidth(const StyleParam& _styleParam, float& width, float& slope) {

    // NB: 0.5 because 'width' will be extruded in both directions
    float tileRes = 0.5 / m_tileSizePixels;

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
            width = widthMeterToPixel(m_zoom, m_tileSizePixels, width);
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

template <class V>
void PolylineStyleBuilder<V>::addFeature(const Feature& _feat, const DrawRule& _rule) {

    if (_feat.geometryType == GeometryType::points) { return; }
    if (!checkRule(_rule)) { return; }

    Parameters params = parseRule(_rule, _feat.props);

    if (params.fill.width[0] <= 0.0f && params.fill.width[1] <= 0.0f ) { return; }

    if (_feat.geometryType == GeometryType::lines) {
        // Line geometries are never clipped to tiles, so keep all segments
        params.keepTileEdges = true;

        for (auto& line : _feat.lines) {
            addMesh(line, params);
        }
    } else {
        params.closedPolygon = true;

        for (auto& polygon : _feat.polygons) {
            for (const auto& line : polygon) {
                addMesh(line, params);
            }
        }
    }
}

template <class V>
void PolylineStyleBuilder<V>::buildLine(const Line& _line, const typename Parameters::Attributes& _att,
                        MeshData<V>& _mesh) {

    m_builder.addVertex = [&_mesh, &_att](const glm::vec3& coord,
                                   const glm::vec2& normal,
                                   const glm::vec2& uv) {
        _mesh.vertices.push_back({{ coord.x,coord.y }, normal, uv,
                              _att.width, _att.height, _att.color});
    };

    Builders::buildPolyLine(_line, m_builder);

    _mesh.indices.insert(_mesh.indices.end(),
                         m_builder.indices.begin(),
                         m_builder.indices.end());

    _mesh.offsets.emplace_back(m_builder.indices.size(),
                               m_builder.numVertices);

    m_builder.clear();
}

template <class V>
void PolylineStyleBuilder<V>::addMesh(const Line& _line, const Parameters& _params) {

    m_builder.cap = _params.fill.cap;
    m_builder.join = _params.fill.join;
    m_builder.miterLimit = _params.fill.miterLimit;
    m_builder.keepTileEdges = _params.keepTileEdges;
    m_builder.closedPolygon = _params.closedPolygon;

    if (_params.lineOn) { buildLine(_line, _params.fill, m_meshData[0]); }

    if (!_params.outlineOn) { return; }

    if (!_params.lineOn ||
        _params.stroke.cap != _params.fill.cap ||
        _params.stroke.join != _params.fill.join ||
        _params.stroke.miterLimit != _params.fill.miterLimit) {
        // need to re-triangulate with different cap and/or join
        m_builder.cap = _params.stroke.cap;
        m_builder.join = _params.stroke.join;
        m_builder.miterLimit = _params.stroke.miterLimit;

        buildLine(_line, _params.stroke, m_meshData[1]);

    } else {
        auto& fill = m_meshData[0];
        auto& stroke = m_meshData[1];

        // reuse indices from original line, overriding color and width
        size_t nIndices = fill.offsets.back().first;
        size_t nVertices = fill.offsets.back().second;
        stroke.offsets.emplace_back(nIndices, nVertices);

        auto indicesIt = fill.indices.end() - nIndices;
        stroke.indices.insert(stroke.indices.end(),
                                 indicesIt,
                                 fill.indices.end());

        auto vertexIt = fill.vertices.end() - nVertices;

        glm::vec2 width = _params.stroke.width;
        GLuint abgr = _params.stroke.color;
        short order = _params.stroke.height[1];

        for (; vertexIt != fill.vertices.end(); ++vertexIt) {
            stroke.vertices.emplace_back(*vertexIt, order, width, abgr);
        }
    }
}

std::unique_ptr<StyleBuilder> PolylineStyle::createBuilder() const {
    if (m_texCoordsGeneration) {
        auto builder = std::make_unique<PolylineStyleBuilder<PolylineVertex>>(*this);
        builder->polylineBuilder().useTexCoords = true;
        return std::move(builder);
    } else {
        auto builder = std::make_unique<PolylineStyleBuilder<PolylineVertexNoUVs>>(*this);
        builder->polylineBuilder().useTexCoords = false;
        return std::move(builder);
    }
}

}
