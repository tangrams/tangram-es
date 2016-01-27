#include "polylineStyle.h"

#include "tangram.h"
#include "platform.h"
#include "material.h"
#include "gl/shaderProgram.h"
#include "gl/typedMesh.h"
#include "scene/stops.h"
#include "scene/drawRule.h"
#include "tile/tile.h"
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

    PolylineVertex(glm::vec3 position, float order, glm::vec2 uv,
                   glm::vec2 extrude, glm::vec2 width, GLuint abgr)
        : pos(glm::i16vec4{ glm::round(position * position_scale), order * order_scale }),
          texcoord(uv * texture_scale),
          extrude(extrude * extrusion_scale, width * extrusion_scale),
          abgr(abgr) {}

    PolylineVertex(PolylineVertex v, float order, glm::vec2 width, GLuint abgr)
        : pos(glm::i16vec4{ glm::round(glm::vec3(v.pos)), order * order_scale}),
          texcoord(v.texcoord),
          extrude(glm::i16vec4{ v.extrude.x, v.extrude.y, width * extrusion_scale }),
          abgr(abgr) {}

    glm::i16vec4 pos;
    glm::u16vec2 texcoord;
    glm::i16vec4 extrude;
    GLuint abgr;
};

using Mesh = TypedMesh<PolylineVertex>;

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

VboMesh* PolylineStyle::newMesh() const {
    return new Mesh(m_vertexLayout, m_drawMode);
}

void PolylineStyle::buildPolygon(const Polygon& _poly, const DrawRule& _rule, const Properties& _props,
                                 VboMesh& _mesh, Tile& _tile) const {

    auto params = parseRule(_rule, _props, _tile);
    for (const auto& line : _poly) {
        buildMesh(line, params, _mesh);
    }
}

void PolylineStyle::buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props,
                              VboMesh& _mesh, Tile& _tile) const {

    auto params = parseRule(_rule, _props, _tile);
    params.keepTileEdges = true; // Line geometries are never clipped to tiles, so keep all segments
    buildMesh(_line, params, _mesh);

}

double widthMeterToPixel(int _zoom, double _tileSize, double _width) {
    // pixel per meter at z == 0
    double meterRes = _tileSize / (2.0 * MapProjection::HALF_CIRCUMFERENCE);
    // pixel per meter at zoom
    meterRes *= exp2(_zoom);

    return _width * meterRes;
}

bool evalStyleParamWidth(StyleParamKey _key, const DrawRule& _rule, const Tile& _tile,
                         float& width, float& dWdZ){

    int zoom  = _tile.getID().z;
    double tileSize = _tile.getProjection()->TileSize();

    // NB: 0.5 because 'width' will be extruded in both directions
    double tileRes = 0.5 / tileSize;


    auto& styleParam = _rule.findParameter(_key);
    if (styleParam.stops) {

        width = styleParam.value.get<float>();
        width *= tileRes;

        dWdZ = styleParam.stops->evalWidth(zoom + 1);
        dWdZ *= tileRes;
        // NB: Multiply by 2 for the outline to get the expected stroke pixel width.
        if (_key == StyleParamKey::outline_width) {
            width *= 2;
            dWdZ *= 2;
        }

        dWdZ -= width;

        return true;
    }

    if (styleParam.value.is<StyleParam::Width>()) {
        auto& widthParam = styleParam.value.get<StyleParam::Width>();

        width = widthParam.value;

        if (widthParam.isMeter()) {
            width = widthMeterToPixel(zoom, tileSize, width);
            width *= tileRes;
            dWdZ = width * 2;
        } else {
            width *= tileRes;
            dWdZ = width;
        }

        if (_key == StyleParamKey::outline_width) {
            width *= 2;
            dWdZ *= 2;
        }

        dWdZ -= width;

        return true;
    }

    LOGD("Invalid type for Width '%d'\n", styleParam.value.which());
    return false;
}

PolylineStyle::Parameters PolylineStyle::parseRule(const DrawRule& _rule, const Properties& _props, const Tile& _tile) const {
    Parameters p;

    uint32_t cap = 0, join = 0;

    _rule.get(StyleParamKey::color, p.fill.color);
    _rule.get(StyleParamKey::cap, cap);
    _rule.get(StyleParamKey::join, join);
    _rule.get(StyleParamKey::order, p.fill.order);

    p.fill.cap = static_cast<CapTypes>(cap);
    p.fill.join = static_cast<JoinTypes>(join);

    evalStyleParamWidth(StyleParamKey::width, _rule, _tile, p.fill.width, p.fill.slope);

    p.outline.order = p.fill.order; // will offset from fill later

    if (_rule.get(StyleParamKey::outline_color, p.outline.color) |
        _rule.get(StyleParamKey::outline_order, p.outline.order) |
        _rule.contains(StyleParamKey::outline_width) |
        _rule.get(StyleParamKey::outline_cap, cap) |
        _rule.get(StyleParamKey::outline_join, join)) {
        p.outlineOn = true;
        p.outline.cap = static_cast<CapTypes>(cap);
        p.outline.join = static_cast<JoinTypes>(join);
        evalStyleParamWidth(StyleParamKey::outline_width, _rule, _tile, p.outline.width, p.outline.slope);
    }

    Extrude extrude;
    _rule.get(StyleParamKey::extrude, extrude);
    p.height = getUpperExtrudeMeters(extrude, _props) * _tile.getInverseScale();

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        p.fill.color = p.fill.color << (_tile.getID().z % 6);
        p.outline.color = p.outline.color << (_tile.getID().z % 6);
    }

    _rule.get(StyleParamKey::tile_edges, p.keepTileEdges);

    return p;
}

void PolylineStyle::buildMesh(const Line& _line, Parameters& _params, VboMesh& _mesh) const {

    std::vector<PolylineVertex> vertices;

    GLuint color = _params.fill.color;
    float width = _params.fill.width;
    float slope = _params.fill.slope;
    float order = _params.fill.order;

    if (width <= 0.f && slope <= 0.f ) { return; }

    PolyLineBuilder builder {
        [&](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& texCoord) {
            glm::vec3 position = glm::vec3{coord.x, coord.y, _params.height};
            vertices.push_back({position, order, texCoord, normal, { width, slope }, color});
        },
        [&](size_t sizeHint){ vertices.reserve(sizeHint); },
        _params.fill.cap,
        _params.fill.join,
        _params.keepTileEdges
    };

    Builders::buildPolyLine(_line, builder);

    if (_params.outlineOn && (_params.outline.width > 0.f || _params.outline.slope > 0.f)) {

        // Must update existing variables, they are captured (and used) by the builder
        color = _params.outline.color;
        width += _params.outline.width;
        slope += _params.outline.slope;
        order = std::min(_params.outline.order, _params.fill.order) - 0.5f;

        if (_params.outline.cap != _params.fill.cap || _params.outline.join != _params.fill.join) {
            // need to re-triangulate with different cap and/or join
            builder.cap = _params.outline.cap;
            builder.join = _params.outline.join;
            Builders::buildPolyLine(_line, builder);
        } else {
            // re-use indices and positions from original line
            size_t oldSize = builder.indices.size();
            size_t offset = vertices.size();
            builder.indices.reserve(2 * oldSize);

            for(size_t i = 0; i < oldSize; i++) {
                builder.indices.push_back(offset + builder.indices[i]);
            }
            for (size_t i = 0; i < offset; i++) {
                vertices.push_back({ vertices[i], order, { width, slope }, color });
            }
        }
    }

    auto& mesh = static_cast<Mesh&>(_mesh);
    mesh.addVertices(std::move(vertices), std::move(builder.indices));
}

}
