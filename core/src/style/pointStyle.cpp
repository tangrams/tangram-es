#include "pointStyle.h"

#include "platform.h"
#include "material.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"
#include "gl/vertexLayout.h"
#include "labels/labelMesh.h"
#include "labels/spriteLabel.h"
#include "scene/drawRule.h"
#include "scene/spriteAtlas.h"
#include "tile/tile.h"
#include "util/builders.h"
#include "view/view.h"
#include "data/propertyItem.h" // Include wherever Properties is used!
#include "scene/stops.h"

constexpr float texture_scale = 65535.f;

namespace Tangram {

PointStyle::PointStyle(std::string _name, Blending _blendMode, GLenum _drawMode)
    : Style(_name, _blendMode, _drawMode) {}

PointStyle::~PointStyle() {}

void PointStyle::constructVertexLayout() {

    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_SHORT, false, 0},
        {"a_uv", 2, GL_UNSIGNED_SHORT, true, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_extrude", 2, GL_SHORT, false, 0},
        {"a_screenPosition", 2, GL_SHORT, false, 0},
        {"a_alpha", 1, GL_SHORT, true, 0},
        {"a_rotation", 1, GL_SHORT, false, 0},
    }));
}

void PointStyle::constructShaderProgram() {

    std::string fragShaderSrcStr = stringFromFile("shaders/point.fs", PathType::internal);
    std::string vertShaderSrcStr = stringFromFile("shaders/point.vs", PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    std::string defines;

    if (!m_spriteAtlas && !m_texture) {
        defines += "#define TANGRAM_POINT\n";
    }

    m_shaderProgram->addSourceBlock("defines", defines);
}

void PointStyle::onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit) {
    if (m_spriteAtlas) {
        m_spriteAtlas->bind(0);
    } else if (m_texture) {
        m_texture->update(0);
        m_texture->bind(0);
    }

    m_shaderProgram->setUniformi("u_tex", 0);
    m_shaderProgram->setUniformMatrix4f("u_ortho", _view.getOrthoViewportMatrix());

    Style::onBeginDrawFrame(_view, _scene, 1);
}

struct PointStyleBuilder : public StyleBuilder {

    const PointStyle& m_style;

    std::vector<Label::Vertex> m_vertices;
    std::vector<std::unique_ptr<Label>> m_labels;

    std::unique_ptr<LabelMesh> m_mesh;
    float m_zoom;

    void setup(const Tile& _tile) override {
        m_zoom = _tile.getID().z;
        m_mesh = std::make_unique<LabelMesh>(m_style.vertexLayout(), m_style.drawMode());
    }

    bool checkRule(const DrawRule& _rule) const override;

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;

    std::unique_ptr<StyledMesh> build() override {
        m_mesh->compile(m_labels, m_vertices);
        m_labels.clear();
        m_vertices.clear();
        return std::move(m_mesh);
    };

    const Style& style() const override { return m_style; }

    PointStyleBuilder(const PointStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    void pushQuad(std::vector<Label::Vertex>& _vertices, const glm::vec2& _size, const glm::vec2& _uvBL,
                  const glm::vec2& _uvTR, unsigned int _color, float _extrudeScale) const;

    bool getUVQuad(PointStyle::Parameters& _params, glm::vec4& _quad) const;

    PointStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    template<typename ...Args>
    void addLabel(Args&& ...args) {
        m_labels.push_back(std::make_unique<SpriteLabel>(args...));
    }

};

bool PointStyleBuilder::checkRule(const DrawRule& _rule) const {
    uint32_t checkColor;
    // require a color or texture atlas/texture to be valid
    if (!_rule.get(StyleParamKey::color, checkColor) &&
        !m_style.texture() &&
        !m_style.spriteAtlas()) {
        return false;
    }
    return true;
}

auto PointStyleBuilder::applyRule(const DrawRule& _rule, const Properties& _props) const -> PointStyle::Parameters {

    PointStyle::Parameters p;
    glm::vec2 size;
    std::string anchor;

    _rule.get(StyleParamKey::color, p.color);
    _rule.get(StyleParamKey::sprite, p.sprite);
    _rule.get(StyleParamKey::offset, p.labelOptions.offset);
    _rule.get(StyleParamKey::priority, p.labelOptions.priority);
    _rule.get(StyleParamKey::sprite_default, p.spriteDefault);
    _rule.get(StyleParamKey::centroid, p.centroid);
    _rule.get(StyleParamKey::interactive, p.labelOptions.interactive);
    _rule.get(StyleParamKey::collide, p.labelOptions.collide);
    _rule.get(StyleParamKey::transition_hide_time, p.labelOptions.hideTransition.time);
    _rule.get(StyleParamKey::transition_selected_time, p.labelOptions.selectTransition.time);
    _rule.get(StyleParamKey::transition_show_time, p.labelOptions.showTransition.time);
    _rule.get(StyleParamKey::anchor, anchor);

    auto sizeParam = _rule.findParameter(StyleParamKey::size);
    if (sizeParam.stops && sizeParam.value.is<float>()) {
        float lowerSize = sizeParam.value.get<float>();
        float higherSize = sizeParam.stops->evalWidth(m_zoom + 1);
        p.extrudeScale = (higherSize - lowerSize) * 0.5f - 1.f;
        p.size = glm::vec2(lowerSize);
    } else if (_rule.get(StyleParamKey::size, size)) {
        if (size.x == 0.f || std::isnan(size.y)) {
            p.size = glm::vec2(size.x);
        } else {
            p.size = size;
        }
    } else {
        p.size = glm::vec2(NAN, NAN);
    }

    LabelProperty::anchor(anchor, p.anchor);

    if (p.labelOptions.interactive) {
        p.labelOptions.properties = std::make_shared<Properties>(_props);
    }

    std::hash<PointStyle::Parameters> hash;
    p.labelOptions.paramHash = hash(p);

    return p;
}

void PointStyleBuilder::pushQuad(std::vector<Label::Vertex>& _vertices, const glm::vec2& _size,
                                 const glm::vec2& _uvBL, const glm::vec2& _uvTR, unsigned int _color,
                                 float _extrudeScale) const {

    float es = _extrudeScale;

    // Attribute will be normalized - scale to max short;
    glm::vec2 uvTR = _uvTR * texture_scale;
    glm::vec2 uvBL = _uvBL * texture_scale;

    _vertices.push_back({{    0.0,       0.0}, {uvBL.x, uvTR.y}, {-es,  es }, _color});
    _vertices.push_back({{_size.x,       0.0}, {uvTR.x, uvTR.y}, { es,  es }, _color});
    _vertices.push_back({{    0.0,  -_size.y}, {uvBL.x, uvBL.y}, {-es, -es }, _color});
    _vertices.push_back({{_size.x,  -_size.y}, {uvTR.x, uvBL.y}, { es, -es }, _color});
}

bool PointStyleBuilder::getUVQuad(PointStyle::Parameters& _params, glm::vec4& _quad) const {
    _quad = glm::vec4(0.0, 0.0, 1.0, 1.0);

    if (m_style.spriteAtlas()) {
        SpriteNode spriteNode;

        if (!m_style.spriteAtlas()->getSpriteNode(_params.sprite, spriteNode) &&
            !m_style.spriteAtlas()->getSpriteNode(_params.spriteDefault, spriteNode)) {
            return false;
        }

        if (std::isnan(_params.size.x)) {
            _params.size = spriteNode.m_size;
        }

        _quad.x = spriteNode.m_uvBL.x;
        _quad.y = spriteNode.m_uvBL.y;
        _quad.z = spriteNode.m_uvTR.x;
        _quad.w = spriteNode.m_uvTR.y;
    } else {
        // default point size
        if (std::isnan(_params.size.x)) {
            _params.size = glm::vec2(8.0);
        }
    }

    _params.size *= m_style.pixelScale();

    return true;
}

void PointStyleBuilder::addPoint(const Point& _point, const Properties& _props,
                                 const DrawRule& _rule) {

    PointStyle::Parameters p = applyRule(_rule, _props);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return;
    }

    Label::Transform transform = { glm::vec2(_point) };

    addLabel(transform, p.size, *m_mesh, m_vertices.size(),
             p.labelOptions, p.extrudeScale, p.anchor);

    pushQuad(m_vertices, p.size,
             {uvsQuad.x, uvsQuad.y},
             {uvsQuad.z, uvsQuad.w},
             p.color, p.extrudeScale);
}

void PointStyleBuilder::addLine(const Line& _line, const Properties& _props,
                                const DrawRule& _rule) {

    PointStyle::Parameters p = applyRule(_rule, _props);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return;
    }

    for (size_t i = 0; i < _line.size(); ++i) {
        Label::Transform transform = { glm::vec2(_line[i]) };

        addLabel(transform, p.size, *m_mesh, m_vertices.size(),
                 p.labelOptions, p.extrudeScale, p.anchor);

        pushQuad(m_vertices, p.size,
                 {uvsQuad.x, uvsQuad.y},
                 {uvsQuad.z, uvsQuad.w},
                 p.color, p.extrudeScale);
    }
}

void PointStyleBuilder::addPolygon(const Polygon& _polygon, const Properties& _props,
                                   const DrawRule& _rule) {

    PointStyle::Parameters p = applyRule(_rule, _props);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return;
    }

    if (!p.centroid) {

        int size = 0;
        for (auto line : _polygon) { size += line.size(); }

        for (auto line : _polygon) {
            for (auto point : line) {
                Label::Transform transform = { glm::vec2(point) };

                addLabel(transform, p.size, *m_mesh, m_vertices.size(),
                         p.labelOptions, p.extrudeScale, p.anchor);

                pushQuad(m_vertices, p.size,
                         {uvsQuad.x, uvsQuad.y},
                         {uvsQuad.z, uvsQuad.w},
                         p.color, p.extrudeScale);
            }
        }
    } else {
        glm::vec2 c = centroid(_polygon);
        Label::Transform transform = { c };

        addLabel(transform, p.size, *m_mesh, m_vertices.size(),
                 p.labelOptions, p.extrudeScale, p.anchor);

        pushQuad(m_vertices, p.size,
                 {uvsQuad.x, uvsQuad.y},
                 {uvsQuad.z, uvsQuad.w},
                 p.color, p.extrudeScale);
    }
}

std::unique_ptr<StyleBuilder> PointStyle::createBuilder() const {
    return std::make_unique<PointStyleBuilder>(*this);
}

}
