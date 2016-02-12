#include "pointStyle.h"

#include "platform.h"
#include "material.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"
#include "gl/vertexLayout.h"
#include "gl/renderState.h"
#include "labels/labelMesh.h"
#include "labels/spriteLabel.h"
#include "scene/drawRule.h"
#include "scene/spriteAtlas.h"
#include "labels/labelSet.h"
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
        {"a_screen_position", 2, GL_SHORT, false, 0},
        {"a_alpha", 1, GL_UNSIGNED_BYTE, true, 0},
        {"a_scale", 1, GL_UNSIGNED_BYTE, false, 0},
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

    m_mesh = std::make_unique<LabelMesh>(m_vertexLayout, m_drawMode);
}

void PointStyle::onBeginFrame() {
    // Upload meshes for next frame
    m_mesh->myUpload();
}

void PointStyle::onBeginDrawFrame(const View& _view, Scene& _scene) {
    Style::onBeginDrawFrame(_view, _scene);

    if (m_spriteAtlas) {
        m_spriteAtlas->bind(RenderState::nextAvailableTextureUnit());
    } else if (m_texture) {
        m_texture->update(RenderState::nextAvailableTextureUnit());
        m_texture->bind(RenderState::currentTextureUnit());
    }

    m_shaderProgram->setUniformi(m_uTex, RenderState::currentTextureUnit());
    m_shaderProgram->setUniformMatrix4f(m_uOrtho, _view.getOrthoViewportMatrix());

    m_mesh->draw(*m_shaderProgram);
    m_mesh->clear();
}

struct PointStyleBuilder : public StyleBuilder {

    struct PointStyleMesh : public LabelMesh, public LabelSet {
        using LabelMesh::LabelMesh;
    };

    const PointStyle& m_style;

    std::vector<std::unique_ptr<Label>> m_labels;
    std::vector<SpriteQuad> m_quads;

    std::unique_ptr<SpriteLabels> m_spriteLabels;
    float m_zoom;

    void setup(const Tile& _tile) override {
        m_zoom = _tile.getID().z;
        m_spriteLabels = std::make_unique<SpriteLabels>(m_style);
    }

    bool checkRule(const DrawRule& _rule) const override;

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;

    std::unique_ptr<StyledMesh> build() override {
        if (m_labels.empty()) { return nullptr; }

        m_spriteLabels->setLabels(m_labels);
        m_spriteLabels->setQuads(m_quads);
        m_labels.clear();
        m_quads.clear();
        return std::move(m_spriteLabels);
    };

    const Style& style() const override { return m_style; }

    PointStyleBuilder(const PointStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    bool getUVQuad(PointStyle::Parameters& _params, glm::vec4& _quad) const;

    PointStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    void addLabel(const Point& _point, const glm::vec4& _quad,
                  const PointStyle::Parameters& _params);

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

void PointStyleBuilder::addLabel(const Point& _point, const glm::vec4& _quad,
                                 const PointStyle::Parameters& _params) {

    m_labels.push_back(std::make_unique<SpriteLabel>(Label::Transform{glm::vec2(_point)},
                                                     _params.size,
                                                     _params.labelOptions,
                                                     _params.extrudeScale,
                                                     _params.anchor,
                                                     *m_spriteLabels,
                                                     m_quads.size()));

    glm::i16vec2 size = _params.size * position_scale;

    // Attribute will be normalized - scale to max short;
    glm::vec2 uvTR = glm::vec2{_quad.z, _quad.w} * texture_scale;
    glm::vec2 uvBL = glm::vec2{_quad.x, _quad.y} * texture_scale;

    int16_t extrude = _params.extrudeScale * extrusion_scale;

    m_quads.push_back({
            {{{0, 0},
             {uvBL.x, uvTR.y},
             {-extrude, extrude}},
            {{size.x, 0},
             {uvTR.x, uvTR.y},
             {extrude, extrude}},
            {{0, -size.y},
             {uvBL.x, uvBL.y},
             {-extrude, -extrude}},
            {{size.x, -size.y},
             {uvTR.x, uvBL.y},
             {extrude, -extrude}}},
            _params.color});
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

    addLabel(_point, uvsQuad, p);
}

void PointStyleBuilder::addLine(const Line& _line, const Properties& _props,
                                const DrawRule& _rule) {

    PointStyle::Parameters p = applyRule(_rule, _props);
    glm::vec4 uvsQuad;

    if (!getUVQuad(p, uvsQuad)) {
        return;
    }

    for (size_t i = 0; i < _line.size(); ++i) {
        addLabel(_line[i], uvsQuad, p);
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
        for (auto line : _polygon) {
            for (auto point : line) {
                addLabel(point, uvsQuad, p);
            }
        }
    } else {
        glm::vec2 c = centroid(_polygon);

        addLabel(Point{c,0}, uvsQuad, p);
    }
}

std::unique_ptr<StyleBuilder> PointStyle::createBuilder() const {
    return std::make_unique<PointStyleBuilder>(*this);
}

}
