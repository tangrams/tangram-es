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

namespace {

struct Mesh : public LabelMesh {
    const size_t maxVertices = 16384;

    Mesh(std::shared_ptr<VertexLayout> _vertexLayout)
        : LabelMesh(_vertexLayout, GL_TRIANGLES) {}

    void setLabels(std::vector<std::unique_ptr<Label>>& _labels,
                   std::vector<Label::Vertex>& _vertices) {

        typedef std::vector<std::unique_ptr<Label>>::iterator iter_t;

        m_labels.reserve(_labels.size());
        m_labels.insert(m_labels.begin(),
                      std::move_iterator<iter_t>(_labels.begin()),
                      std::move_iterator<iter_t>(_labels.end()));

        // Compile vertex buffer directly instead of making a temporary copy
        m_nVertices = _vertices.size();

        int stride = m_vertexLayout->getStride();
        m_glVertexData = new GLbyte[stride * m_nVertices];
        std::memcpy(m_glVertexData,
                    reinterpret_cast<const GLbyte*>(_vertices.data()),
                    m_nVertices * stride);

        for (size_t offset = 0; offset < m_nVertices; offset += maxVertices) {
            size_t nVertices = maxVertices;
            if (offset + maxVertices > m_nVertices) {
                nVertices = m_nVertices - offset;
            }
            m_vertexOffsets.emplace_back(nVertices / 4 * 6, nVertices);
        }
        m_isCompiled = true;
    }

    virtual void compileVertexBuffer() override {
        // already compiled above
    }
};

struct Builder : public StyleBuilder {

    // FIXME - holds GL resources
    std::shared_ptr<FontContext> m_fontContext;

    bool m_sdf;
    float m_pixelScale;

    virtual void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    virtual void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    virtual void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;
    virtual bool checkRule(const DrawRule& _rule) const override;

    virtual void initMesh() override {
        m_vertices.clear();
        m_labels.clear();
        m_mesh = std::make_unique<Mesh>(m_vertexLayout);
    }
    virtual std::unique_ptr<VboMesh> build() override {
        if (!m_labels.empty()) {
            m_mesh->setLabels(m_labels, m_vertices);
        }
        return std::move(m_mesh);
    };

    Builder(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode,
            std::shared_ptr<FontContext> _fontContext, bool _sdf, float _pixelScale)
        : StyleBuilder(_vertexLayout, _drawMode),
          m_fontContext(_fontContext),
          m_sdf(_sdf),
          m_pixelScale(_pixelScale) {}

    TextStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    struct {
        std::vector<Label::Vertex> vertices;
        glm::vec2 bbox;
        glm::vec2 quadsLocalOrigin;
        int numLines;
        FontContext::FontMetrics metrics;

    } m_scratch;

    std::unique_ptr<Mesh> m_mesh;

    std::vector<Label::Vertex> m_vertices;
    std::vector<std::unique_ptr<Label>> m_labels;

    bool prepareLabel(const TextStyle::Parameters& _params, Label::Type _type);
    void addLabel(const TextStyle::Parameters& _params, Label::Type _type, Label::Transform _transform);
};

bool Builder::checkRule(const DrawRule& _rule) const {
    return true;
}

auto Builder::applyRule(const DrawRule& _rule, const Properties& _props) const -> TextStyle::Parameters {
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
        if (!m_fontContext->lock()) { return p; }

        p.fontId = m_fontContext->addFont(fontFamily, fontWeight, fontStyle);

        m_fontContext->unlock();
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

    if (_rule.get(StyleParamKey::interactive, p.interactive) && p.interactive) {
        p.properties = std::make_shared<Properties>(_props);
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

void Builder::addPoint(const Point& _point, const Properties& _props, const DrawRule& _rule) {

    TextStyle::Parameters params = applyRule(_rule, _props);

    if (!params.isValid()) { return; }

    if (!prepareLabel(params, Label::Type::point)) { return; }

    addLabel(params, Label::Type::point, { glm::vec2(_point), glm::vec2(_point) });
}

void Builder::addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) {

    TextStyle::Parameters params = applyRule(_rule, _props);

    if (!params.isValid()) { return; }

    if (!prepareLabel(params, Label::Type::line)) { return; }

    //LOGE("label lenght %f - %f", m_scratch.bbox.x, m_tile->getProjection()->TileSize());

    float pixel = 2.0 / m_tile->getProjection()->TileSize();
    float minLength = m_scratch.bbox.x * pixel * 0.2;

    for (size_t i = 0; i < _line.size() - 1; i++) {
        glm::vec2 p1 = glm::vec2(_line[i]);
        glm::vec2 p2 = glm::vec2(_line[i + 1]);
        if (glm::length(p1-p2) > minLength) {
            addLabel(params, Label::Type::line, { p1, p2 });
        }
    }
}

void Builder::addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) {
    Point p = glm::vec3(centroid(_polygon), 0.0);
    addPoint(p, _props, _rule);
}

bool Builder::prepareLabel(const TextStyle::Parameters& _params, Label::Type _type) {

    if (_params.fontId < 0 || _params.fontSize <= 0.f || _params.text.size() == 0) {
        return false;
    }

    /// Apply text transforms
    const std::string* renderText;
    std::string text;

    if (_params.transform == TextLabelProperty::Transform::none) {
        renderText = &_params.text;
    } else {
        text = TextBuffer::applyTextTransform(_params, _params.text);
        renderText = &text;
    }

    if (!m_fontContext->lock()) {
        return false;
    }

    /// Rasterize the glyphs
    auto& quads = m_fontContext->rasterize(*renderText, _params.fontId,
                                           _params.fontSize, _params.blurSpread);

    size_t numGlyphs = quads.size();

    if (numGlyphs == 0) {
        m_fontContext->unlock();
        return false;
    }

    // Stroke width is normalized by the distance of the SDF spread, then scaled
    // to a char, then packed into the "alpha" channel of stroke. The .25 scaling
    // probably has to do with how the SDF is generated, but honestly I'm not sure
    // what it represents.
    uint32_t strokeWidth = _params.strokeWidth / _params.blurSpread * 255. * .25;
    uint32_t stroke = (_params.strokeColor & 0x00ffffff) + (strokeWidth << 24);

    m_scratch.metrics = m_fontContext->getMetrics();
    m_scratch.bbox = glm::vec2(0);

    /// Apply word wrapping
    std::vector<TextBuffer::WordBreak> wordBreaks;
    int nLine = TextBuffer::applyWordWrapping(quads, _params, m_scratch.metrics, _type, wordBreaks);

    /// Generate the quads
    float yMin = std::numeric_limits<float>::max();
    float xMin = std::numeric_limits<float>::max();

    m_scratch.vertices.clear();

    for (int i = 0; i < int(quads.size()); ++i) {
        if (wordBreaks.size() > 0) {
            bool skip = false;
            // Skip spaces/CR quads
            for (int j = 0; j < int(wordBreaks.size()) - 1; ++j) {
                const auto& b1 = wordBreaks[j];
                const auto& b2 = wordBreaks[j + 1];
                if (i >= b1.end + 1 && i <= b2.start - 1) {
                    skip = true;
                    break;
                }
            }
            if (skip) { continue; }
        }

        const auto& q = quads[i];
        m_scratch.vertices.push_back({{q.x0, q.y0}, {q.s0, q.t0}, _params.fill, stroke});
        m_scratch.vertices.push_back({{q.x0, q.y1}, {q.s0, q.t1}, _params.fill, stroke});
        m_scratch.vertices.push_back({{q.x1, q.y0}, {q.s1, q.t0}, _params.fill, stroke});
        m_scratch.vertices.push_back({{q.x1, q.y1}, {q.s1, q.t1}, _params.fill, stroke});

        yMin = std::min(yMin, q.y0);
        xMin = std::min(xMin, q.x0);

        m_scratch.bbox.x = std::max(m_scratch.bbox.x, q.x1);
        m_scratch.bbox.y = std::max(m_scratch.bbox.y, std::abs(yMin - q.y1));
    }

    m_scratch.bbox.x -= xMin;
    m_scratch.quadsLocalOrigin = { xMin, quads[0].y0 };
    m_scratch.numLines = nLine;

    m_fontContext->unlock();

    return true;
}

void Builder::addLabel(const TextStyle::Parameters& _params, Label::Type _type, Label::Transform _transform) {
    int vertexOffset = m_vertices.size();
    int numVertices = m_scratch.vertices.size();

    m_labels.emplace_back(new TextLabel(_transform, _type, m_scratch.bbox, *m_mesh,
                                        { vertexOffset, numVertices },
                                        _params.labelOptions, m_scratch.metrics, m_scratch.numLines,
                                        _params.anchor, m_scratch.quadsLocalOrigin));

    m_vertices.insert(m_vertices.end(),
                      m_scratch.vertices.begin(),
                      m_scratch.vertices.end());
}

}

std::unique_ptr<StyleBuilder> TextStyle::createBuilder() const {
    return std::make_unique<Builder>(m_vertexLayout, m_drawMode, m_fontContext, m_sdf, m_pixelScale);
}

}
