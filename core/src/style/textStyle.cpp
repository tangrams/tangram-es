#include "textStyle.h"

#include "material.h"
#include "scene/drawRule.h"
#include "tile/tile.h"
#include "gl/shaderProgram.h"
#include "gl/vboMesh.h"
#include "gl/texture.h"
#include "gl/renderState.h"
#include "view/view.h"
#include "data/propertyItem.h" // Include wherever Properties is used!
#include "labels/labelMesh.h"
#include "labels/textLabel.h"
#include "text/fontContext.h"

#include <mutex>
#include <locale>

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
        {"a_screenPosition", 2, GL_SHORT, false, 0},
        {"a_alpha", 1, GL_UNSIGNED_BYTE, true, 0},
        {"a_scale", 1, GL_UNSIGNED_BYTE, false, 0},
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

void TextStyle::onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit) {

    m_shaderProgram->setUniformf("u_uv_scale_factor",
                                 glm::vec2(1.0f / textureSize));

    m_shaderProgram->setUniformf("u_max_stroke_width",
                                 m_context->maxStrokeWidth());

    m_shaderProgram->setUniformi("u_tex", 0);
    m_shaderProgram->setUniformMatrix4f("u_ortho", _view.getOrthoViewportMatrix());

    Style::onBeginDrawFrame(_view, _scene, 1);
}

void TextStyle::onEndDrawFrame() {

    if (m_sdf) {
        m_shaderProgram->setUniformi("u_pass", 1);
        for (size_t i = 0; i < m_meshes.size(); i++) {
            if (m_meshes[i]->isReady()) {
                m_context->bindTexture(i, 0);
                m_meshes[i]->draw(*m_shaderProgram, false);
            }
        }
        m_shaderProgram->setUniformi("u_pass", 0);
    }

    for (size_t i = 0; i < m_meshes.size(); i++) {
        if (m_meshes[i]->isReady()) {
            m_context->bindTexture(i, 0);
            m_meshes[i]->draw(*m_shaderProgram, true);
        }
    }
}

void TextStyle::onBeginUpdate() {
    // Ensure that meshes are available to push to
    // in labels::update()
    size_t s = m_context->glyphBatchCount();
    while (m_meshes.size() < s) {
        m_meshes.push_back(std::make_unique<LabelMesh>(m_vertexLayout, GL_TRIANGLES));
    }
}

void TextStyle::onBeginFrame() {
    m_context->updateTextures();

    // Upload meshes
    for (size_t i = 0; i < m_meshes.size(); i++) {
        m_meshes[i]->myUpload();
    }
}

struct TextBatch : public alf::TextBatch {
    using Base = alf::TextBatch;
    using Base::Base;

    alf::LineMetrics drawWithLineWrappingByCharacterCount(const alf::LineLayout& _line, size_t _maxChar,
                                                   TextLabelProperty::Align _alignment) {
        static std::vector<std::pair<int,float>> lineWraps;

        lineWraps.clear();

        float lineWidth = 0;
        float maxWidth = 0;
        size_t charCount = 0;
        size_t shapeCount = 0;

        float lastWidth = 0;
        size_t lastShape = 0;
        size_t lastChar = 0;

        for (auto& c : _line.shapes()) {
            shapeCount++;
            lineWidth += _line.advance(c);

            if (c.cluster) { charCount++; }

            if (c.canBreak || c.mustBreak) {
                lastShape = shapeCount;
                lastChar = charCount;
                lastWidth = lineWidth;
            }

            if (c.mustBreak || charCount > _maxChar) {
                // only go to next line if chars have been added on the current line
                // HACK: avoid short words on single line
                if (lastShape != 0 && (c.mustBreak || shapeCount - lastShape > 4 )) {

                    auto& endShape = _line.shapes()[lastShape-1];
                    if (endShape.isSpace) {
                        lineWidth -= _line.advance(endShape);
                        lastWidth -= _line.advance(endShape);
                    }
                    lineWraps.emplace_back(lastShape, lastWidth);
                    maxWidth = std::max(maxWidth, lastWidth);

                    lineWidth -= lastWidth;
                    charCount -= lastChar;

                    lastShape = 0;
                }
            }
        }
        if (charCount > 0) {
            lineWraps.emplace_back(shapeCount, lineWidth);
            maxWidth = std::max(maxWidth, lineWidth);
        }

        size_t shapeStart = 0;
        alf::LineMetrics lineMetrics;
        glm::vec2 position;
        for (auto wrap : lineWraps) {
            switch(_alignment) {
            case TextLabelProperty::Align::center:
                position.x = (maxWidth - wrap.second) * 0.5;
                break;
            case TextLabelProperty::Align::right:
                position.x = (maxWidth - wrap.second);
                break;
            default:
                position.x = 0;
            }

            size_t shapeEnd = wrap.first;

            alf::TextBatch::draw(_line, shapeStart, shapeEnd, position, lineMetrics);
            shapeStart = shapeEnd;

            position.y += _line.height();
        }

        return lineMetrics;
    }
};

class TextStyleBuilder : public StyleBuilder {

public:

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

    virtual void setup(const Tile& _tile) override {
        m_tileSize = _tile.getProjection()->TileSize();
        m_scratch.clear();

        m_textLabels = std::make_unique<TextLabels>(m_style);
    }

    virtual std::unique_ptr<StyledMesh> build() override {
        if (!m_scratch.labels.empty()) {
            m_textLabels->setLabels(m_scratch.labels);
            m_textLabels->setQuads(m_scratch.quads);
        }

        m_scratch.clear();

        return std::move(m_textLabels);
    };

    TextStyle::Parameters applyRule(const DrawRule& _rule,
                                    const Properties& _props) const;

    bool prepareLabel(TextStyle::Parameters& _params, Label::Type _type);
    void addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                  Label::Transform _transform);

    std::string applyTextTransform(const TextStyle::Parameters& _params,
                                   const std::string& _string);

protected:

    struct ScratchBuffer : public alf::MeshCallback {
        std::vector<GlyphQuad> quads;
        std::vector<std::unique_ptr<Label>> labels;

        // label width and height
        glm::vec2 bbox;
        glm::vec2 quadsLocalOrigin;
        int numLines;
        TextLabel::FontMetrics metrics;
        int numQuads;

        uint32_t fill;
        uint32_t stroke;
        uint8_t fontScale;

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
                    atlasGlyph.atlas,
                    {{glm::vec2{q.x1, q.y1} * position_scale, {g.u1, g.v1}},
                     {glm::vec2{q.x1, q.y2} * position_scale, {g.u1, g.v2}},
                     {glm::vec2{q.x2, q.y1} * position_scale, {g.u2, g.v1}},
                     {glm::vec2{q.x2, q.y2} * position_scale, {g.u2, g.v2}}}});
        }

        void clear() {
            quads.clear();
            labels.clear();
        }
    };

    const TextStyle& m_style;

    float m_tileSize;

    alf::TextShaper m_shaper;
    TextBatch m_batch;

    bool m_sdf;
    float m_pixelScale = 1;

    ScratchBuffer m_scratch;

    std::unique_ptr<TextLabels> m_textLabels;

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

std::string TextStyleBuilder::applyTextTransform(const TextStyle::Parameters& _params,
                                                 const std::string& _string) {
    std::locale loc;
    std::string text = _string;

    switch (_params.transform) {
        case TextLabelProperty::Transform::capitalize:
            text[0] = toupper(text[0], loc);
            if (text.size() > 1) {
                for (size_t i = 1; i < text.length(); ++i) {
                    if (text[i - 1] == ' ') {
                        text[i] = std::toupper(text[i], loc);
                    }
                }
            }
            break;
        case TextLabelProperty::Transform::lowercase:
            for (size_t i = 0; i < text.length(); ++i) {
                text[i] = std::tolower(text[i], loc);
            }
            break;
        case TextLabelProperty::Transform::uppercase:
            // TODO : use to wupper when any wide character is detected
            for (size_t i = 0; i < text.length(); ++i) {
                text[i] = std::toupper(text[i], loc);
            }
            break;
        default:
            break;
    }

    return text;
}

bool TextStyleBuilder::prepareLabel(TextStyle::Parameters& _params, Label::Type _type) {

    if (_params.text.empty() || _params.fontSize <= 0.f) {
        LOGD("invalid params: '%s' %f", _params.text.c_str(), _params.fontSize);
        return false;
    }

    m_scratch.reset();

    // Apply text transforms
    const std::string* renderText;
    std::string text;

    if (_params.transform == TextLabelProperty::Transform::none) {
        renderText = &_params.text;
    } else {
        text = applyTextTransform(_params, _params.text);
        renderText = &text;
    }

    // Scale factor by which the texture glyphs are scaled to match fontSize
    float fontScale = (_params.fontSize * m_style.pixelScale()) / _params.font->size();

    // Stroke width is normalized by the distance of the SDF spread, then
    // scaled to a char, then packed into the "alpha" channel of stroke.
    // Maximal strokeWidth is 3px, attribute is normalized to 0-1 range.
    float strokeWidth = _params.strokeWidth * m_style.pixelScale();

#if 0
    // HACK - need to use a smaller font in this case
    // to have enough sdf-radius for the stroke!
    // if (strokeWidth > 1.5 * fontScale) {
    //     strokeWidth = 1.5 * fontScale;
    // }

    //// see point.vs and sdf.fs
    float sdf_radius = 3.0;

    // (rate of change within one pixel)
    float sdf_pixel = 0.5 / sdf_radius;

    // scale strokeWidth to sdf_pixel
    float stroke_width = strokeWidth * sdf_pixel;

    // scale sdf (texture is scaled depeding on font size)
    stroke_width /= fontScale;

    float v_sdf_threshold = 0.5 - stroke_width;

    // 0.1245 antialiasing filter width of unscaled glyph
    float filter_width = sdf_pixel / fontScale * (0.5 + 0.25);

    if (v_sdf_threshold - filter_width < 0) {
        // modify stroke width to be within sdf_radius
        LOG("size:%f scale:%f stroke:%f att:%f thres:%f filter:%f ==> %f",
            _params.fontSize, fontScale, _params.strokeWidth,
            stroke_width, v_sdf_threshold, filter_width,
            v_sdf_threshold - filter_width);
    }
#endif
    auto ctx = m_style.context();

    uint32_t strokeAttrib = std::max(std::min(strokeWidth / ctx->maxStrokeWidth() * 255.f, 255.f), 0.f);

    m_scratch.stroke = (_params.strokeColor & 0x00ffffff) + (strokeAttrib << 24);
    m_scratch.fill = _params.fill;
    m_scratch.fontScale = std::min(fontScale * 64.f, 255.f);

    {
        std::lock_guard<std::mutex> lock(ctx->m_mutex);
        auto line = m_shaper.shape(_params.font, *renderText);

        LOG("fontScale %d", m_scratch.fontScale);
        line.setScale(fontScale);

        alf::LineMetrics lineMetrics;

        if (_type == Label::Type::point && _params.wordWrap) {
            //auto adv = m_batch.draw(line, {0, 0}, _params.maxLineWidth * line.height() * 0.5);
            lineMetrics = m_batch.drawWithLineWrappingByCharacterCount(line, _params.maxLineWidth,
                                                                          _params.align);
        } else {
            m_batch.draw(line, glm::vec2(0.0), lineMetrics);
        }

        // FIXME: bbox should account for local origin (negative (x,y) position
        m_scratch.bbox.x = std::fabsf(lineMetrics.aabb.x) + (lineMetrics.aabb.z);
        m_scratch.bbox.y = std::fabsf(lineMetrics.aabb.y) + (lineMetrics.aabb.w);

        m_scratch.numLines = m_scratch.bbox.y / line.height();

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

    m_scratch.labels.emplace_back(new TextLabel(_transform, _type,
                                                _params.labelOptions,
                                                _params.anchor,
                                                {m_scratch.fill, m_scratch.stroke, m_scratch.fontScale },
                                                m_scratch.bbox,
                                                m_scratch.metrics,
                                                m_scratch.numLines,
                                                m_scratch.quadsLocalOrigin,
                                                *m_textLabels, { quadOffset, numQuads }));
}

TextStyle::Parameters TextStyleBuilder::applyRule(const DrawRule& _rule,
                                           const Properties& _props) const {

    const static std::string key_name("name");
    const static std::string defaultWeight("400");
    const static std::string defaultStyle("normal");
    const static std::string defaultFamily("default");

    TextStyle::Parameters p;
    glm::vec2 offset;

    _rule.get(StyleParamKey::text_source, p.text);
    if (!_rule.isJSFunction(StyleParamKey::text_source)) {
        if (p.text.empty()) {
            p.text = _props.getString(key_name);
        } else {
            p.text = _props.getString(p.text);
        }
    }
    if (p.text.empty()) { return p; }

    auto fontFamily = _rule.get<std::string>(StyleParamKey::font_family);
    fontFamily = (!fontFamily) ? &defaultFamily : fontFamily;

    auto fontWeight = _rule.get<std::string>(StyleParamKey::font_weight);
    fontWeight = (!fontWeight) ? &defaultWeight : fontWeight;

    auto fontStyle = _rule.get<std::string>(StyleParamKey::font_style);
    fontStyle = (!fontStyle) ? &defaultStyle : fontStyle;

    _rule.get(StyleParamKey::font_size, p.fontSize);
    // TODO - look font from fontManager
    p.font = m_style.context()->getFont(*fontFamily, *fontStyle, *fontWeight,
                                        p.fontSize * m_style.pixelScale());

    _rule.get(StyleParamKey::font_fill, p.fill);
    _rule.get(StyleParamKey::offset, p.labelOptions.offset);
    _rule.get(StyleParamKey::font_stroke_color, p.strokeColor);
    _rule.get(StyleParamKey::font_stroke_width, p.strokeWidth);
    _rule.get(StyleParamKey::priority, p.labelOptions.priority);
    _rule.get(StyleParamKey::collide, p.labelOptions.collide);
    _rule.get(StyleParamKey::transition_hide_time, p.labelOptions.hideTransition.time);
    _rule.get(StyleParamKey::transition_selected_time, p.labelOptions.selectTransition.time);
    _rule.get(StyleParamKey::transition_show_time, p.labelOptions.showTransition.time);
    _rule.get(StyleParamKey::text_wrap, p.maxLineWidth);

    size_t repeatGroupHash = 0;
    std::string repeatGroup;
    if (_rule.get(StyleParamKey::repeat_group, repeatGroup)) {
        hash_combine(repeatGroupHash, repeatGroup);
    } else {
        // Default to hash on all used layer names ('draw.key' in JS version)
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

    if (auto* anchor = _rule.get<std::string>(StyleParamKey::anchor)) {
        LabelProperty::anchor(*anchor, p.anchor);
    }

    if (auto* transform = _rule.get<std::string>(StyleParamKey::transform)) {
        TextLabelProperty::transform(*transform, p.transform);
    }

    if (auto* align = _rule.get<std::string>(StyleParamKey::align)) {
        bool res = TextLabelProperty::align(*align, p.align);
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

class DebugTextStyleBuilder : public TextStyleBuilder {

public:

    DebugTextStyleBuilder(const TextStyle& _style) :
        TextStyleBuilder(_style) {}

    void setup(const Tile& _tile) override;

    std::unique_ptr<StyledMesh> build() override;

private:
    std::string m_tileID;

};

void DebugTextStyleBuilder::setup(const Tile& _tile) {
    if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
        return;
    }

    m_tileID = _tile.getID().toString();

    TextStyleBuilder::setup(_tile);
}

std::unique_ptr<StyledMesh> DebugTextStyleBuilder::build() {
    if (!Tangram::getDebugFlag(Tangram::DebugFlags::tile_infos)) {
        return nullptr;
    }

    TextStyle::Parameters params;

    params.text = m_tileID;
    params.fontSize = 30.f;

    params.font = m_style.context()->getFont("sans-serif", "normal", "400", 32 * m_style.pixelScale());

    if (!prepareLabel(params, Label::Type::debug)) {
        return nullptr;
    }

    addLabel(params, Label::Type::debug, { glm::vec2(.5f) });

    if (!m_scratch.labels.empty()) {
        m_textLabels->setLabels(m_scratch.labels);
        m_textLabels->setQuads(m_scratch.quads);
    }

    m_scratch.clear();

    return std::move(m_textLabels);
}

std::unique_ptr<StyleBuilder> DebugTextStyle::createBuilder() const {
    return std::make_unique<DebugTextStyleBuilder>(*this);
}

}
