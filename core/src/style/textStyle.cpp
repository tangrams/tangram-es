#include "textStyle.h"

#include "material.h"
#include "scene/drawRule.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/mesh.h"
#include "gl/texture.h"
#include "gl/renderState.h"
#include "view/view.h"
#include "data/propertyItem.h" // Include wherever Properties is used!
#include "labels/labelMesh.h"
#include "labels/labelContainer.h"
#include "labels/textLabel.h"
#include "text/fontContext.h"

#include <mutex>

#include "platform.h"
#include "tangram.h"

namespace Tangram {

TextStyle::TextStyle(std::string _name, bool _sdf, Blending _blendMode, GLenum _drawMode) :
    Style(_name, _blendMode, _drawMode), m_sdf(_sdf),
    m_context(std::make_shared<AlfonsContext>())
{
}

TextStyle::~TextStyle() {}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_SHORT, false, 0},
        {"a_uv", 2, GL_UNSIGNED_SHORT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_stroke", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_screen_position", 2, GL_SHORT, false, 0},
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

    m_shaderProgram->addSourceBlock("defines", defines);
}

void TextStyle::onBeginDrawFrame(const View& _view, Scene& _scene) {
    m_context->updateTextures();

    for (auto& mesh : m_meshes) { mesh->myUpload(); }

    m_shaderProgram->setUniformf(m_uTexScaleFactor,
                                 glm::vec2(1.0f / textureSize));
    m_shaderProgram->setUniformi(m_uTex, 0);
    m_shaderProgram->setUniformMatrix4f(m_uOrtho, _view.getOrthoViewportMatrix());

    Style::onBeginDrawFrame(_view, _scene);
}

void TextStyle::onEndDrawFrame() {

    if (m_sdf) {
        m_shaderProgram->setUniformi(m_uPass, 1);
        for (size_t i = 0; i < m_meshes.size(); i++) {
            if (m_meshes[i]->compiled()) {
                m_context->bindTexture(i, 0);

                m_meshes[i]->draw(*m_shaderProgram);
            }
        }
        m_shaderProgram->setUniformi(m_uPass, 0);
    }

    for (size_t i = 0; i < m_meshes.size(); i++) {
        if (m_meshes[i]->compiled()) {
            m_context->bindTexture(i, 0);

            m_meshes[i]->draw(*m_shaderProgram);

            // FIXME - resets for next frame..
            // should be done only when buffers are changing
            m_meshes[i]->clear();
        }
    }
}

void TextStyle::onUpdate() {
    size_t s = m_context->glyphBatchCount();
    while (m_meshes.size() < s) {
        m_meshes.push_back(std::make_unique<TextMesh>(m_vertexLayout, GL_TRIANGLES));
    }
}

struct TextStyleBuilder : public StyleBuilder {

    const TextStyle& m_style;

    float m_tileSize;

    alf::TextShaper m_shaper;
    alf::TextBatch m_batch;

    bool m_sdf;
    float m_pixelScale = 1;

    std::unique_ptr<LabelContainer> m_labelContainer;
    std::vector<std::unique_ptr<Label>> m_labels;

    struct ScratchBuffer : public alf::MeshCallback {
        std::vector<GlyphQuad> quads;

        // label width and height
        glm::vec2 bbox;
        glm::vec2 quadsLocalOrigin;
        int numLines;
        FontMetrics metrics;
        int numQuads;

        uint32_t fill;
        uint32_t stroke;

        float yMin, xMin;

        void reset() {
            yMin = std::numeric_limits<float>::max();
            xMin = std::numeric_limits<float>::max();
            bbox = glm::vec2(0);
            numLines = 1;
            numQuads = 0;
        }
        // TextRenderer interface
        void drawGlyph(const alf::Quad& q, const alf::AtlasGlyph& altasGlyph) override {}
        void drawGlyph(const alf::Rect& q, const alf::AtlasGlyph& atlasGlyph) override {
            numQuads++;

            auto& g = *atlasGlyph.glyph;
            quads.push_back({
                    {{glm::vec2{q.x1, q.y1} * position_scale, {g.u1, g.v1}},
                     {glm::vec2{q.x1, q.y2} * position_scale, {g.u1, g.v2}},
                     {glm::vec2{q.x2, q.y1} * position_scale, {g.u2, g.v1}},
                     {glm::vec2{q.x2, q.y2} * position_scale, {g.u2, g.v2}}},
                    fill, stroke, atlasGlyph.atlas });
        }

        void clear() {
            quads.clear();
        }
    };

    ScratchBuffer m_scratch;

    TextStyleBuilder(const TextStyle& _style) :
        StyleBuilder(_style),
        m_style(_style),
        m_batch(_style.context()->m_atlas, m_scratch) {}

    const Style& style() const override { return m_style; }

    // StyleBuilder interface
    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;
    bool checkRule(const DrawRule& _rule) const override;

    void setup(const Tile& _tile) override {
        m_tileSize = _tile.getProjection()->TileSize();
        m_labels.clear();
        m_scratch.clear();

        m_labelContainer = std::make_unique<LabelContainer>(m_style);
    }

    virtual std::unique_ptr<StyledMesh> build() override {
        if (!m_labels.empty()) {
            m_labelContainer->setLabels(m_labels);
            m_labelContainer->setQuads(m_scratch.quads);
        }

        m_labels.clear();
        m_scratch.clear();

        return std::move(m_labelContainer);
    };

    TextStyle::Parameters applyRule(const DrawRule& _rule,
                                    const Properties& _props) const;

    bool prepareLabel(const TextStyle::Parameters& _params, Label::Type _type);
    void addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                  Label::Transform _transform);
};

bool TextStyleBuilder::checkRule(const DrawRule& _rule) const {
    return true;
}


void TextStyleBuilder::addPoint(const Point& _point,
                               const Properties& _props,
                               const DrawRule& _rule) {
    TextStyle::Parameters params = applyRule(_rule, _props);

    if (!prepareLabel(params, Label::Type::point)) { return; }

    addLabel(params, Label::Type::point, { glm::vec2(_point), glm::vec2(_point) });
}

void TextStyleBuilder::addLine(const Line& _line,
                              const Properties& _props,
                              const DrawRule& _rule) {

    TextStyle::Parameters params = applyRule(_rule, _props);

    if (!prepareLabel(params, Label::Type::line)) { return; }

    float pixel = 2.0 / (m_tileSize * m_style.pixelScale());

    float minLength = m_scratch.bbox.x * pixel * 0.2;

    for (size_t i = 0; i < _line.size() - 1; i++) {
        glm::vec2 p1 = glm::vec2(_line[i]);
        glm::vec2 p2 = glm::vec2(_line[i + 1]);
        if (glm::length(p1-p2) > minLength) {
            addLabel(params, Label::Type::line, { p1, p2 });
        }
    }
}

void TextStyleBuilder::addPolygon(const Polygon& _polygon,
                                 const Properties& _props,
                                 const DrawRule& _rule) {
    Point p = glm::vec3(centroid(_polygon), 0.0);
    addPoint(p, _props, _rule);
}

bool TextStyleBuilder::prepareLabel(const TextStyle::Parameters& _params, Label::Type _type) {

    if (_params.text.empty() || _params.fontSize <= 0.f) {
        LOGD("invalid params: %s %f", _params.text.c_str(), _params.fontSize);
        return false;
    }

    m_scratch.reset();

    m_scratch.fill = _params.fill;

    // Stroke width is normalized by the distance of the SDF spread, then scaled
    // to a char, then packed into the "alpha" channel of stroke. The .25 scaling
    // probably has to do with how the SDF is generated, but honestly I'm not sure
    // what it represents.
    float fontScale = _params.fontSize / FONT_SIZE * m_style.pixelScale();

    //uint32_t strokeWidth = (_params.strokeWidth / _params.blurSpread * 255. * .25) / fontScale;
    //m_scratch.stroke = (_params.strokeColor & 0x00ffffff) + (strokeWidth << 24);

    uint32_t strokeWidth = (_params.strokeWidth / 3.f * 255. * .25) / fontScale;

    m_scratch.stroke = (_params.strokeColor & 0x00ffffff) + (strokeWidth << 24);

    {
        auto ctx = m_style.context();

        std::lock_guard<std::mutex> lock(ctx->m_mutex);
        auto line = m_shaper.shape(ctx->m_font, _params.text);

        line.setScale(fontScale);

        if (_type == Label::Type::point) {
            auto adv = m_batch.draw(line, {0, 0}, _params.maxLineWidth * line.height() * 0.5);

            m_scratch.numLines = adv.y/line.height();
            m_scratch.bbox.y = adv.y;
            m_scratch.bbox.x = adv.x;

        } else {
            m_batch.draw(line, {0, 0});

            m_scratch.bbox.y = line.height();
            m_scratch.bbox.x = line.advance();
        }

        m_scratch.metrics.descender = -line.descent();
        m_scratch.metrics.ascender = line.ascent();
        m_scratch.metrics.lineHeight = line.height();
        m_scratch.quadsLocalOrigin = { 0, -line.ascent() };

    }

    // LOG("LABEL  %f %f - %f %f - %d",
    //     m_scratch.metrics.ascender, m_scratch.metrics.descender,
    //     m_scratch.bbox.x, m_scratch.bbox.y,
    //     m_scratch.vertices.size());

    return true;
}

void TextStyleBuilder::addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                       Label::Transform _transform) {

    int numQuads = m_scratch.numQuads;
    int quadOffset = m_scratch.quads.size() - numQuads;

    m_labels.emplace_back(new TextLabel(_transform, _type,
                                          m_scratch.bbox, *m_labelContainer,
                                          { quadOffset, numQuads },
                                          _params.labelOptions,
                                          m_scratch.metrics,
                                          m_scratch.numLines,
                                          _params.anchor,
                                          m_scratch.quadsLocalOrigin));
}

TextStyle::Parameters TextStyleBuilder::applyRule(const DrawRule& _rule,
                                                  const Properties& _props) const {

    const static std::string key_name("name");

    TextStyle::Parameters p;

    std::string fontFamily, fontWeight, fontStyle, transform, align, anchor;
    glm::vec2 offset;

    _rule.get(StyleParamKey::font_family, fontFamily);
    _rule.get(StyleParamKey::font_weight, fontWeight);
    _rule.get(StyleParamKey::font_style, fontStyle);

    fontWeight = (fontWeight.size() == 0) ? "400" : fontWeight;
    fontStyle = (fontStyle.size() == 0) ? "normal" : fontStyle;
    // TODO - look font from fontManager

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
        repeatGroupHash = _rule.getParamSetHash();
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
    p.fontSize *= m_pixelScale;
    p.labelOptions.offset *= m_pixelScale;
    float emSize = p.fontSize / 16.f;
    p.blurSpread = m_sdf ? emSize * 5.0f : 0.0f;

    float boundingBoxBuffer = -p.fontSize / 2.f;
    p.labelOptions.buffer = boundingBoxBuffer;

    std::hash<TextStyle::Parameters> hash;
    p.labelOptions.paramHash = hash(p);

    return p;
}

std::unique_ptr<StyleBuilder> TextStyle::createBuilder() const {
    return std::make_unique<TextStyleBuilder>(*this);
}

}
