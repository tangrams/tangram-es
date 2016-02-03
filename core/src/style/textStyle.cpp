#include "textStyle.h"

#include "material.h"
#include "scene/drawRule.h"
#include "text/fontContext.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/vboMesh.h"
#include "view/view.h"
#include "labels/textLabel.h"
#include "text/fontContext.h"
#include "data/propertyItem.h" // Include wherever Properties is used!
#include "text/textBuffer.h"
#include "util/hash.h"

#include "platform.h"
#include "tangram.h"

namespace Tangram {

TextStyle::TextStyle(std::string _name, std::shared_ptr<FontContext> _fontContext, bool _sdf,
                     bool _sdfMultisampling, Blending _blendMode, GLenum _drawMode) :
    Style(_name, _blendMode, _drawMode), m_sdf(_sdf), m_sdfMultisampling(_sdfMultisampling),
    m_fontContext(_fontContext) {
}

TextStyle::~TextStyle() {
}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_SHORT, false, 0},
        {"a_uv", 2, GL_UNSIGNED_SHORT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_stroke", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_screenPosition", 2, GL_SHORT, false, 0},
        {"a_alpha", 1, GL_SHORT, true, 0},
        {"a_rotation", 1, GL_SHORT, false, 0},
    }));
}

void TextStyle::constructShaderProgram() {
    std::string frag = m_sdf ? "shaders/sdf.fs" : "shaders/text.fs";

    std::string vertShaderSrcStr = stringFromFile("shaders/point.vs", PathType::internal);
    std::string fragShaderSrcStr = stringFromFile(frag.c_str(), PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    std::string defines = "#define TANGRAM_TEXT\n";

    if (m_sdf && m_sdfMultisampling) {
        defines += "#define TANGRAM_SDF_MULTISAMPLING\n";
    }

    m_shaderProgram->addSourceBlock("defines", defines);
}

void TextStyle::onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit) {
    m_fontContext->bindAtlas(0);

    m_shaderProgram->setUniformf("u_uv_scale_factor",
                                 1.0f / m_fontContext->getAtlasResolution());
    m_shaderProgram->setUniformi("u_tex", 0);
    m_shaderProgram->setUniformMatrix4f("u_ortho", _view.getOrthoViewportMatrix());

    Style::onBeginDrawFrame(_view, _scene, 1);
}

struct TextStyleBuilder : public StyleBuilder {

    const TextStyle& m_style;

    float m_tileSize;

    void setup(const Tile& _tile) override {
        m_builder.setup(m_style.vertexLayout());
        m_tileSize = _tile.getProjection()->TileSize();
    }

    bool checkRule(const DrawRule& _rule) const override;

    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;

    std::unique_ptr<VboMesh> build() override {
        return m_builder.build();
    }

    const Style& style() const override { return m_style; }

    TextStyleBuilder(const TextStyle& _style) : StyleBuilder(_style), m_style(_style) {}

    TextStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    TextBuffer::Builder m_builder;

    bool prepareLabel(const TextStyle::Parameters& _params, Label::Type _type);
    void addLabel(const TextStyle::Parameters& _params, Label::Type _type, Label::Transform _transform);
};

bool TextStyleBuilder::checkRule(const DrawRule& _rule) const {
    return true;
}

auto TextStyleBuilder::applyRule(const DrawRule& _rule, const Properties& _props) const -> TextStyle::Parameters {
    const static std::string key_name("name");

    TextStyle::Parameters p;

    std::string fontFamily, fontWeight, fontStyle, transform, align, anchor;
    glm::vec2 offset;

    _rule.get(StyleParamKey::font_family, fontFamily);
    _rule.get(StyleParamKey::font_weight, fontWeight);
    _rule.get(StyleParamKey::font_style, fontStyle);

    fontWeight = (fontWeight.size() == 0) ? "400" : fontWeight;
    fontStyle = (fontStyle.size() == 0) ? "normal" : fontStyle;
    {
        auto& fontContext = m_style.fontContext();
        if (!fontContext.lock()) { return p; }

        p.fontId = fontContext.addFont(fontFamily, fontWeight, fontStyle);

        fontContext.unlock();
        if (p.fontId < 0) { return p; }
    }

    _rule.get(StyleParamKey::font_size, p.fontSize);
    _rule.get(StyleParamKey::font_fill, p.fill);
    _rule.get(StyleParamKey::offset, p.labelOptions.offset);
    _rule.get(StyleParamKey::font_stroke_color, p.strokeColor);
    _rule.get(StyleParamKey::font_stroke_width, p.strokeWidth);
    _rule.get(StyleParamKey::transform, transform);
    _rule.get(StyleParamKey::align, align);
    _rule.get(StyleParamKey::anchor, anchor);
    _rule.get(StyleParamKey::priority, p.labelOptions.priority);
    _rule.get(StyleParamKey::collide, p.labelOptions.collide);
    _rule.get(StyleParamKey::transition_hide_time, p.labelOptions.hideTransition.time);
    _rule.get(StyleParamKey::transition_selected_time, p.labelOptions.selectTransition.time);
    _rule.get(StyleParamKey::transition_show_time, p.labelOptions.showTransition.time);
    _rule.get(StyleParamKey::text_wrap, p.maxLineWidth);

    _rule.get(StyleParamKey::text_source, p.text);
    if (!_rule.isJSFunction(StyleParamKey::text_source)) {
        if (p.text.empty()) {
            p.text = _props.getString(key_name);
        } else {
            p.text = _props.getString(p.text);
        }
    }

    size_t repeatGroupHash = 0;
    std::string repeatGroup;
    if (_rule.get(StyleParamKey::repeat_group, repeatGroup)) {
        hash_combine(repeatGroupHash, repeatGroup);
    } else {
        // Default to hash on all used layer names ('draw.key' in JS version)
        for (auto* name : _rule.getLayerNames()) {
            hash_combine(repeatGroupHash, name);
            // repeatGroup += name;
            // repeatGroup += "/";
        }
        //LOG("rg: %s", p.labelOptions.repeatGroup.c_str());
    }

    StyleParam::Width repeatDistance;
    if (_rule.get(StyleParamKey::repeat_distance, repeatDistance)) {
        p.labelOptions.repeatDistance = repeatDistance.value;
    } else {
        p.labelOptions.repeatDistance = View::s_pixelsPerTile;
    }

    hash_combine(repeatGroupHash, p.text);
    p.labelOptions.repeatGroup = repeatGroupHash;

    p.labelOptions.repeatDistance *= m_style.pixelScale();

    if (_rule.get(StyleParamKey::interactive, p.interactive) && p.interactive) {
        p.labelOptions.properties = std::make_shared<Properties>(_props);
    }

    LabelProperty::anchor(anchor, p.anchor);

    TextLabelProperty::transform(transform, p.transform);
    bool res = TextLabelProperty::align(align, p.align);
    if (!res) {
        switch(p.anchor) {
            case LabelProperty::Anchor::top_left:
            case LabelProperty::Anchor::left:
            case LabelProperty::Anchor::bottom_left:
                p.align = TextLabelProperty::Align::right;
                break;
            case LabelProperty::Anchor::top_right:
            case LabelProperty::Anchor::right:
            case LabelProperty::Anchor::bottom_right:
                p.align = TextLabelProperty::Align::left;
                break;
            case LabelProperty::Anchor::top:
            case LabelProperty::Anchor::bottom:
            case LabelProperty::Anchor::center:
                break;
        }
    }

    /* Global operations done for fontsize and sdfblur */
    p.fontSize *= m_style.pixelScale();
    p.labelOptions.offset *= m_style.pixelScale();
    float emSize = p.fontSize / 16.f;
    p.blurSpread = m_style.useSDF() ? emSize * 5.0f : 0.0f;

    float boundingBoxBuffer = -p.fontSize / 2.f;
    p.labelOptions.buffer = boundingBoxBuffer;

    std::hash<TextStyle::Parameters> hash;
    p.labelOptions.paramHash = hash(p);

    return p;
}

void TextStyleBuilder::addPoint(const Point& _point, const Properties& _props, const DrawRule& _rule) {

    TextStyle::Parameters params = applyRule(_rule, _props);

    if (!params.isValid()) { return; }

    if (!m_builder.prepareLabel(m_style.fontContext(), params)) { return; }

    m_builder.addLabel(params, Label::Type::point, { glm::vec2(_point), glm::vec2(_point) });
}

void TextStyleBuilder::addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) {

    TextStyle::Parameters params = applyRule(_rule, _props);

    if (!params.isValid()) { return; }

    // Not yet supported for line labels
    params.wordWrap = false;

    if (!m_builder.prepareLabel(m_style.fontContext(), params)) { return; }

    // Check if any line segment is long enough for the label
    // when the tile is magnified to 2 zoom-levels above (=> 0.25)
    float pixel = 1.0 / (m_tileSize * m_style.pixelScale());
    float minLength = m_builder.labelWidth() * pixel * 0.25;

    for (size_t i = 0; i < _line.size() - 1; i++) {
        glm::vec2 p1 = glm::vec2(_line[i]);
        glm::vec2 p2 = glm::vec2(_line[i + 1]);
        if (glm::length(p1-p2) > minLength) {
            m_builder.addLabel(params, Label::Type::line, { p1, p2 });
        }
    }
}

void TextStyleBuilder::addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) {
    Point p = glm::vec3(centroid(_polygon), 0.0);
    addPoint(p, _props, _rule);
}

std::unique_ptr<StyleBuilder> TextStyle::createBuilder() const {
    return std::make_unique<TextStyleBuilder>(*this);
}

}
