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

namespace {

struct Builder : public StyleBuilder {

    const PolylineStyle& m_style;

    std::unique_ptr<Mesh> m_mesh;
    float m_tileUnitsPerMeter;
    float m_tileSize;
    int m_zoom;

    struct Parameters {
        uint32_t order = 0;
        uint32_t outlineOrder = 0;
        uint32_t color = 0xff00ffff;
        uint32_t outlineColor = 0xff00ffff;
        CapTypes cap = CapTypes::butt;
        CapTypes outlineCap = CapTypes::butt;
        JoinTypes join = JoinTypes::miter;
        JoinTypes outlineJoin = JoinTypes::miter;
        bool outlineOn = false;
        glm::vec2 extrude;
    };

    void begin(const Tile& _tile) override {
        m_tileUnitsPerMeter = _tile.getInverseScale();
        m_zoom = _tile.getID().z;
        m_tileSize = _tile.getProjection()->TileSize();
        m_mesh = std::make_unique<Mesh>(m_style.vertexLayout(), m_style.drawMode());
    }

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;

    std::unique_ptr<VboMesh> build() override {
        m_mesh->compileVertexBuffer();
        return std::move(m_mesh);
    };

    const Style& style() const override { return m_style; }

    Builder(const PolylineStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    bool evalStyleParamWidth(StyleParamKey _key, const DrawRule& _rule,
                             float& width, float& dWdZ);

    static Parameters parseRule(const DrawRule& _rule);
};

auto Builder::parseRule(const DrawRule& _rule) -> Parameters {
    Parameters p;

    uint32_t cap = 0, join = 0;

    _rule.get(StyleParamKey::extrude, p.extrude);
    _rule.get(StyleParamKey::color, p.color);
    _rule.get(StyleParamKey::cap, cap);
    _rule.get(StyleParamKey::join, join);
    _rule.get(StyleParamKey::order, p.order);

    p.cap = static_cast<CapTypes>(cap);
    p.join = static_cast<JoinTypes>(join);

    p.outlineOrder = p.order; // will offset from fill later

    if (_rule.get(StyleParamKey::outline_color, p.outlineColor) |
        _rule.get(StyleParamKey::outline_order, p.outlineOrder) |
        _rule.contains(StyleParamKey::outline_width) |
        _rule.get(StyleParamKey::outline_cap, cap) |
        _rule.get(StyleParamKey::outline_join, join)) {
        p.outlineOn = true;
        p.outlineCap = static_cast<CapTypes>(cap);
        p.outlineJoin = static_cast<JoinTypes>(join);
    }

    return p;
}

void Builder::addPolygon(const Polygon& _poly, const Properties& _props, const DrawRule& _rule) {

    for (const auto& line : _poly) {
        addLine(line, _props, _rule);
    }
}

double widthMeterToPixel(int _zoom, double _tileSize, double _width) {
    // pixel per meter at z == 0
    double meterRes = _tileSize / (2.0 * MapProjection::HALF_CIRCUMFERENCE);
    // pixel per meter at zoom
    meterRes *= exp2(_zoom);

    return _width * meterRes;
}

bool Builder::evalStyleParamWidth(StyleParamKey _key, const DrawRule& _rule,
                                  float& width, float& dWdZ) {

    // NB: 0.5 because 'width' will be extruded in both directions
    double tileRes = 0.5 / m_tileSize;


    auto& styleParam = _rule.findParameter(_key);
    if (styleParam.stops) {

        width = styleParam.value.get<float>();
        width *= tileRes;

        dWdZ = styleParam.stops->evalWidth(m_zoom + 1);
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
            width = widthMeterToPixel(m_zoom, m_tileSize, width);
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

void Builder::addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) {

    std::vector<PolylineVertex> vertices;

    Parameters params = parseRule(_rule);
    GLuint abgr = params.color;

    float dWdZ = 0.f;
    float width = 0.f;

    if (!evalStyleParamWidth(StyleParamKey::width, _rule, width, dWdZ)) {
        return;
    }

    if (width <= 0.0f && dWdZ <= 0.0f ) { return; }

    if (Tangram::getDebugFlag(Tangram::DebugFlags::proxy_colors)) {
        abgr = abgr << (m_zoom % 6);
    }

    auto& extrude = params.extrude;

    float height = getUpperExtrudeMeters(extrude, _props) * m_tileUnitsPerMeter;
    float order = params.order;

    PolyLineBuilder builder {
        [&](const glm::vec3& coord, const glm::vec2& normal, const glm::vec2& texCoord) {
            glm::vec3 position = glm::vec3{coord.x, coord.y, height};
            vertices.push_back({position, order, texCoord, normal, { width, dWdZ }, abgr});
        },
        [&](size_t sizeHint){ vertices.reserve(sizeHint); },
        params.cap,
        params.join
    };

    Builders::buildPolyLine(_line, builder);

    if (params.outlineOn) {

        GLuint abgrOutline = params.outlineColor;
        float outlineOrder = std::min(params.outlineOrder, params.order) - 0.5f;

        float widthOutline = 0.f;
        float dWdZOutline = 0.f;

        if (evalStyleParamWidth(StyleParamKey::outline_width, _rule,
                                widthOutline, dWdZOutline) &&
            ((widthOutline > 0.0f || dWdZOutline > 0.0f)) ) {

            // Note: this must update <width> and <dWdZ> as they are captured
            // (and used) by <builder>
            width += widthOutline;
            dWdZ += dWdZOutline;

            if (params.outlineCap != params.cap || params.outlineJoin != params.join) {
                // need to re-triangulate with different cap and/or join
                builder.cap = params.outlineCap;
                builder.join = params.outlineJoin;
                Builders::buildPolyLine(_line, builder);
            } else {
                // re-use indices from original line
                size_t oldSize = builder.indices.size();
                size_t offset = vertices.size();
                builder.indices.reserve(2 * oldSize);

                for(size_t i = 0; i < oldSize; i++) {
                    builder.indices.push_back(offset + builder.indices[i]);
                }
                for (size_t i = 0; i < offset; i++) {
                    vertices.push_back({ vertices[i], outlineOrder, { width, dWdZ }, abgrOutline });
                }
            }
        }
    }

    m_mesh->addVertices(std::move(vertices), std::move(builder.indices));
}

}

std::unique_ptr<StyleBuilder> PolylineStyle::createBuilder() const {
    return std::make_unique<Builder>(*this);
}

}
