#include "style/polylineStyle.h"

#include "gl/shaderProgram.h"
#include "gl/mesh.h"
#include "gl/texture.h"
#include "gl/renderState.h"
#include "log.h"
#include "map.h"
#include "marker/marker.h"
#include "material.h"
#include "platform.h"
#include "scene/stops.h"
#include "scene/drawRule.h"
#include "tile/tile.h"
#include "util/builders.h"
#include "util/dashArray.h"
#include "util/extrude.h"
#include "util/floatFormatter.h"
#include "util/mapProjection.h"

#include "glm/vec3.hpp"
#include "glm/gtc/type_precision.hpp"

#include "polyline_vs.h"
#include "polyline_fs.h"

constexpr float extrusion_scale = 4096.0f;
constexpr float position_scale = 8192.0f;
constexpr float texture_scale = 8192.0f;
constexpr float order_scale = 2.0f;
constexpr float dash_scale = 20.f;

namespace Tangram {

struct PolylineVertexNoUVs {
    PolylineVertexNoUVs(glm::vec2 position, glm::vec2 extrude, glm::vec2 uv,
                        glm::i16vec2 width, glm::i16vec2 height, GLuint abgr, GLuint selection)
        : pos(glm::i16vec2{ glm::round(position * position_scale)}, height),
          extrude(glm::i16vec2{extrude * extrusion_scale}, width),
          abgr(abgr),
          selection(selection) {}

    PolylineVertexNoUVs(PolylineVertexNoUVs v, short order, glm::i16vec2 width, GLuint abgr, GLuint selection)
        : pos(glm::i16vec4{glm::i16vec3{v.pos}, order}),
          extrude(glm::i16vec4{ v.extrude.x, v.extrude.y, width }),
          abgr(abgr),
          selection(selection) {}

    glm::i16vec4 pos;
    glm::i16vec4 extrude;
    GLuint abgr;
    GLuint selection;
};

struct PolylineVertex : PolylineVertexNoUVs {
    PolylineVertex(glm::vec2 position, glm::vec2 extrude, glm::vec2 uv,
                   glm::i16vec2 width, glm::i16vec2 height, GLuint abgr, GLuint selection)
        : PolylineVertexNoUVs(position, extrude, uv, width, height, abgr, selection),
          texcoord(uv * texture_scale) {}

    PolylineVertex(PolylineVertex v, short order, glm::i16vec2 width, GLuint abgr, GLuint selection)
        : PolylineVertexNoUVs(v, order, width, abgr, selection),
          texcoord(v.texcoord) {}

    glm::u16vec2 texcoord;
};

PolylineStyle::PolylineStyle(std::string _name, Blending _blendMode, GLenum _drawMode, bool _selection)
    : Style(_name, _blendMode, _drawMode, _selection) {
    m_type = StyleType::polyline;
    m_material.material = std::make_shared<Material>();
}

void PolylineStyle::constructVertexLayout() {

    // TODO: Ideally this would be in the same location as the struct that it basically describes
    if (m_texCoordsGeneration) {
        m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 4, GL_SHORT, false, 0},
            {"a_extrude", 4, GL_SHORT, false, 0},
            {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
            {"a_selection_color", 4, GL_UNSIGNED_BYTE, true, 0},
            {"a_texcoord", 2, GL_UNSIGNED_SHORT, false, 0},
        }));
    } else {
        m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
            {"a_position", 4, GL_SHORT, false, 0},
            {"a_extrude", 4, GL_SHORT, false, 0},
            {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
            {"a_selection_color", 4, GL_UNSIGNED_BYTE, true, 0},
        }));
    }
}

void PolylineStyle::onBeginDrawFrame(RenderState& rs, const View& _view) {
    Style::onBeginDrawFrame(rs, _view);

    if (m_texture && m_texture->width() > 0) {
        GLuint textureUnit = rs.nextAvailableTextureUnit();

        m_texture->bind(rs, textureUnit);

        m_shaderProgram->setUniformi(rs, m_uTexture, textureUnit);
        m_shaderProgram->setUniformf(rs, m_uTextureRatio, m_texture->height() / m_texture->width());
    }
}

void PolylineStyle::setDashBackgroundColor(const glm::vec4 _dashBackgroundColor) {
    m_dashBackgroundColor = _dashBackgroundColor;
    m_dashBackground = true;
}

void PolylineStyle::constructShaderProgram() {

    m_shaderSource->setSourceStrings(polyline_fs, polyline_vs);

    if (m_dashArray.size() > 0) {
        TextureOptions options;
        options.minFilter = TextureMinFilter::NEAREST;
        options.magFilter = TextureMagFilter::NEAREST;
        // provides precision for dash patterns that are a fraction of line width
        auto pixels = DashArray::render(m_dashArray, dash_scale);

        m_texture = std::make_shared<Texture>(options);
        m_texture->setPixelData(pixels.size(), 1, sizeof(GLuint),
                                reinterpret_cast<GLubyte*>(pixels.data()),
                                pixels.size() * sizeof(GLuint));

        if (m_dashBackground) {
            m_shaderSource->addSourceBlock("defines", "#define TANGRAM_LINE_BACKGROUND_COLOR vec3(" +
                ff::to_string(m_dashBackgroundColor.r) + ", " +
                ff::to_string(m_dashBackgroundColor.g) + ", " +
                ff::to_string(m_dashBackgroundColor.b) + ")\n");
        }
    }

    if (m_dashArray.size() > 0 || m_texture) {
        m_shaderSource->addSourceBlock("defines", "#define TANGRAM_LINE_TEXTURE\n", false);
        m_shaderSource->addSourceBlock("defines", "#define TANGRAM_ALPHA_TEST 0.25\n", false);
        if (m_dashArray.size() > 0) {
            m_shaderSource->addSourceBlock("defines", "#define TANGRAM_DASHLINE_TEX_SCALE " +
                                            ff::to_string(dash_scale) + "\n", false);
        } else {
            m_shaderSource->addSourceBlock("defines", "#define TANGRAM_DASHLINE_TEX_SCALE 1.0\n", false);
        }
    }

    if (m_texCoordsGeneration) {
        m_shaderSource->addSourceBlock("defines", "#define TANGRAM_USE_TEX_COORDS\n");
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
        uint32_t selectionColor = 0;
    };

    void setup(const Tile& _tile) override;
    void setup(const Marker& _marker, int zoom) override;

    const Style& style() const override { return m_style; }

    bool addFeature(const Feature& _feat, const DrawRule& _rule) override;

    std::unique_ptr<StyledMesh> build() override;

    PolylineStyleBuilder(const PolylineStyle& _style)
        : m_style(_style),
          m_meshData(2) {}

    void addMesh(const Line& _line, const Parameters& _params);

    void buildLine(const Line& _line, const typename Parameters::Attributes& _att,
                   MeshData<V>& _mesh, GLuint _selection);

    Parameters parseRule(const DrawRule& _rule, const Properties& _props);

    bool evalWidth(const StyleParam& _styleParam, float& width, float& slope);

    PolyLineBuilder& polylineBuilder() { return m_builder; }

private:

    const PolylineStyle& m_style;
    PolyLineBuilder m_builder;

    std::vector<MeshData<V>> m_meshData;

    float m_tileUnitsPerMeter = 0;
    float m_tileUnitsPerPixel = 0;
    int m_zoom = 0;
    float m_overzoom2 = 1;
};

template <class V>
void PolylineStyleBuilder<V>::setup(const Tile& tile) {

    const auto& id = tile.getID();

    // Use the 'style zoom' to evaluate style parameters.
    m_zoom = id.s;
    m_overzoom2 = exp2(id.s - id.z);
    m_tileUnitsPerMeter = tile.getInverseScale();
    m_tileUnitsPerPixel = 1.f / MapProjection::tileSize();

    // When a tile is overzoomed, we are actually styling the area of its
    // 'source' tile, which will have a larger effective pixel size at the
    // 'style' zoom level. This scaling is performed in the vertex shader to
    // prevent loss of precision for small dimensions in packed attributes.
}

template <class V>
void PolylineStyleBuilder<V>::setup(const Marker& marker, int zoom) {

    m_zoom = zoom;
    m_overzoom2 = 1.f;
    m_tileUnitsPerMeter = 1.f / marker.extent();
    float metersPerTile = MapProjection::metersPerTileAtZoom(zoom);

    // In general, a Marker won't cover the same area as a tile, so the effective
    // "tile size" for building a Marker is the size of a tile in pixels multiplied
    // by the ratio of the Marker's extent to the length of a tile side at this zoom.
    m_tileUnitsPerPixel = metersPerTile / (marker.extent() * 256.f);

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
    bool outlineVisible = true;
    _rule.get(StyleParamKey::outline_visible, outlineVisible);
    if ( outlineVisible && (!p.lineOn || !_rule.findParameter(StyleParamKey::outline_style)) ) {
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

    p.selectionColor = _rule.selectionColor;

    return p;
}

template <class V>
bool PolylineStyleBuilder<V>::evalWidth(const StyleParam& _styleParam, float& width, float& slope) {

    // NB: 0.5 because 'width' will be extruded in both directions
    float pixelWidthScale = .5f * m_tileUnitsPerPixel;
    float meterWidthScale = .5f * m_tileUnitsPerMeter * m_overzoom2;

    if (_styleParam.stops) {

        width = _styleParam.value.get<float>();
        width *= pixelWidthScale;

        slope = _styleParam.stops->evalExpFloat(m_zoom + 1);
        slope *= pixelWidthScale;
        return true;
    }

    if (_styleParam.value.is<StyleParam::Width>()) {
        auto& widthParam = _styleParam.value.get<StyleParam::Width>();

        width = widthParam.value;

        if (widthParam.isMeter()) {
            width *= meterWidthScale;
            slope = width * 2;
        } else {
            width *= pixelWidthScale;
            slope = width;
        }
        return true;
    }

    LOGD("Invalid type for Width '%d'", _styleParam.value.which());
    return false;
}

template <class V>
bool PolylineStyleBuilder<V>::addFeature(const Feature& _feat, const DrawRule& _rule) {

    if (_feat.geometryType == GeometryType::points) { return false; }
    if (!checkRule(_rule)) { return false; }

    Parameters params = parseRule(_rule, _feat.props);

    if (params.fill.width[0] <= 0.0f && params.fill.width[1] <= 0.0f ) { return false; }

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

    return true;
}

template <class V>
void PolylineStyleBuilder<V>::buildLine(const Line& _line, const typename Parameters::Attributes& _att,
                                        MeshData<V>& _mesh, GLuint selection) {

    float zoom = m_overzoom2;
    m_builder.addVertex = [&](const glm::vec2& coord, const glm::vec2& normal, const glm::vec2& uv) {
        _mesh.vertices.push_back({{ coord.x,coord.y }, normal, { uv.x, uv.y * zoom },
                                  _att.width, _att.height, _att.color, selection});
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

    if (_params.lineOn) { buildLine(_line, _params.fill, m_meshData[0], _params.selectionColor); }

    if (!_params.outlineOn) { return; }

    if (!_params.lineOn ||
        _params.stroke.cap != _params.fill.cap ||
        _params.stroke.join != _params.fill.join ||
        _params.stroke.miterLimit != _params.fill.miterLimit) {
        // need to re-triangulate with different cap and/or join
        m_builder.cap = _params.stroke.cap;
        m_builder.join = _params.stroke.join;
        m_builder.miterLimit = _params.stroke.miterLimit;

        buildLine(_line, _params.stroke, m_meshData[1], _params.selectionColor);

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
            stroke.vertices.emplace_back(*vertexIt, order, width, abgr, _params.selectionColor);
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
