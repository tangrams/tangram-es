#include "alfonsStyle.h"

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

#include "alfons/alfons.h"
#include "alfons/fontManager.h"
#include "alfons/atlas.h"
#include "alfons/textBatch.h"
#include "alfons/textShaper.h"
#include "alfons/font.h"
#include "alfons/inputSource.h"

#include "sdf.h"

#define TEXTURE_SIZE 1024

namespace alf = alfons;

namespace Tangram {

// #define DEFAULT_FONT "fonts/amiri-400regular.ttf"
// #define DEFAULT_FONT "/usr/share/fonts/TTF/DejaVuSans.ttf"
// #define DEFAULT_FONT "fonts/open sans-300normal.ttf"
// #define FALLBACK_FONT "fonts/roboto-regular.ttf"

#define DEFAULT "fonts/NotoSans-Regular.ttf"
#define FONT_AR "fonts/NotoNaskh-Regular.ttf"
#define FONT_HE "fonts/NotoSansHebrew-Regular.ttf"
#define FONT_JA "fonts/DroidSansJapanese.ttf"
#define FALLBACK "fonts/DroidSansFallback.ttf"

#define FONT_SIZE 24

struct AlfonsContext : public alf::TextureCallback {
    AlfonsContext() :
        m_atlas(*this, TEXTURE_SIZE),
        m_texture(TEXTURE_SIZE, TEXTURE_SIZE) {
        m_texData.resize(TEXTURE_SIZE * TEXTURE_SIZE);
        m_font = m_alfons.addFont(DEFAULT, FONT_SIZE);
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FONT_AR), FONT_SIZE));
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FONT_HE), FONT_SIZE));
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FONT_JA), FONT_SIZE));
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FALLBACK), FONT_SIZE));
    }

    void addTexture(alf::AtlasID id, uint16_t width, uint16_t height) override {

    }
    void addGlyph(alf::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
                  const unsigned char* src, uint16_t pad) override {

        // m_texture.setSubData(reinterpret_cast<const GLuint*>(src),
        //                      gx + padding, gy + padding, gw, gh, gw);

        // auto &texData = batches[atlas].texData;
        // auto &dirtyRect = batches[atlas].dirtyRect;
        // auto width = batches[atlas].width;
        uint16_t stride = TEXTURE_SIZE;
        uint16_t width = TEXTURE_SIZE;

        unsigned char* dst = &m_texData[(gx + pad) + (gy + pad) * stride];

        size_t pos = 0;
        for (uint16_t y = 0; y < gh; y++) {
            for (uint16_t x = 0; x < gw; x++) {
                //dst[x + (y * stride)] += std::min(src[pos++] + 30,  0xff);
                dst[x + (y * stride)] = src[pos++];
            }
        }

        dst = &m_texData[gx + gy * width];
        gw += pad * 2;
        gh += pad * 2;

        size_t bytes = gw * gh * sizeof(float) * 3;
        if (m_tmpBuffer.size() < bytes) {
            m_tmpBuffer.resize(bytes);
        }

        sdfBuildDistanceFieldNoAlloc(dst, width, 3,
                                     dst,
                                     gw, gh, width,
                                     &m_tmpBuffer[0]);

        m_texture.setDirty(gy, gy+gh);

        dirty = true;
    }

    alf::FontManager m_alfons;
    alf::GlyphAtlas m_atlas;

    std::shared_ptr<alf::Font> m_font;

    std::vector<unsigned char> m_texData;
    std::vector<unsigned char> m_tmpBuffer;

    Texture m_texture;
    bool dirty;
    std::mutex m_mutex;

};

AlfonsStyle::AlfonsStyle(std::string _name, bool _sdf, Blending _blendMode, GLenum _drawMode) :
    Style(_name, _blendMode, _drawMode), m_sdf(_sdf),
    m_context(std::make_shared<AlfonsContext>()) {}

AlfonsStyle::~AlfonsStyle() {
}

void AlfonsStyle::constructVertexLayout() {
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

void AlfonsStyle::constructShaderProgram() {
    std::string frag = m_sdf ? "shaders/sdf.fs" : "shaders/text.fs";

    std::string vertShaderSrcStr = stringFromFile("shaders/point.vs", PathType::internal);
    std::string fragShaderSrcStr = stringFromFile(frag.c_str(), PathType::internal);

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    std::string defines = "#define TANGRAM_TEXT\n";

    m_shaderProgram->addSourceBlock("defines", defines);
}

void AlfonsStyle::onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit) {
    // m_fontContext->bindAtlas(0);
    if (m_context->dirty) {
        m_context->dirty = false;

        std::lock_guard<std::mutex> lock(m_context->m_mutex);
        m_context->m_texture.update(0, reinterpret_cast<const GLuint*>(m_context->m_texData.data()));
        // m_context->m_texture.update(0);
    }
    m_context->m_texture.bind(0);

    m_shaderProgram->setUniformf("u_uv_scale_factor",
                                 glm::vec2(1.0f / TEXTURE_SIZE));
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

    void compileVertexBuffer() override {
        // already compiled above
    }

    void draw(ShaderProgram& _shader) override {
        // if (m_strokePass) {
        _shader.setUniformi("u_pass", 1);
        LabelMesh::draw(_shader);
        _shader.setUniformi("u_pass", 0);
        //}

        LabelMesh::draw(_shader);
    }

};

struct Builder : public StyleBuilder {

    const AlfonsStyle& m_style;

    float m_tileSize;

    alf::TextShaper m_shaper;
    alf::TextBatch m_batch;

    bool m_sdf;
    float m_pixelScale = 1;

    struct ScratchBuffer : public alf::MeshCallback {
        std::vector<Label::Vertex> vertices;
        // label width and height
        glm::vec2 bbox;
        glm::vec2 quadsLocalOrigin;
        int numLines;
        FontContext::FontMetrics metrics;

        uint32_t fill;
        uint32_t stroke;

        float yMin, xMin;

        void reset() {
            yMin = std::numeric_limits<float>::max();
            xMin = std::numeric_limits<float>::max();
            bbox = glm::vec2(0);
            numLines = 1;
            vertices.clear();
        }
        // TextRenderer interface
        void drawGlyph(const alf::Quad& q, const alf::AtlasGlyph& altasGlyph) override {}
        void drawGlyph(const alf::Rect& q, const alf::AtlasGlyph& atlasGlyph) override {
            auto& g = *atlasGlyph.glyph;
            vertices.push_back({{q.x1, q.y1}, {g.u1, g.v1}, fill, stroke});
            vertices.push_back({{q.x1, q.y2}, {g.u1, g.v2}, fill, stroke});
            vertices.push_back({{q.x2, q.y1}, {g.u2, g.v1}, fill, stroke});
            vertices.push_back({{q.x2, q.y2}, {g.u2, g.v2}, fill, stroke});
        }
    };

    ScratchBuffer m_scratch;
    std::unique_ptr<Mesh> m_mesh;

    std::vector<Label::Vertex> m_vertices;
    std::vector<std::unique_ptr<Label>> m_labels;

    Builder(const AlfonsStyle& _style) :
        StyleBuilder(_style),
        m_style(_style),
        m_batch(_style.context()->m_atlas, m_scratch) {}

    const Style& style() const override { return m_style; }

    // StyleBuilder interface
    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;
    bool checkRule(const DrawRule& _rule) const override;

    void begin(const Tile& _tile) override {
        m_tileSize = _tile.getProjection()->TileSize();
        m_vertices.clear();
        m_labels.clear();
        m_mesh = std::make_unique<Mesh>(m_style.vertexLayout());
    }

    virtual std::unique_ptr<VboMesh> build() override {
        if (!m_labels.empty()) {
            m_mesh->setLabels(m_labels, m_vertices);
        }
        return std::move(m_mesh);
    };

    AlfonsStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    bool prepareLabel(const AlfonsStyle::Parameters& _params, Label::Type _type);
    void addLabel(const AlfonsStyle::Parameters& _params, Label::Type _type, Label::Transform _transform);
};

bool Builder::checkRule(const DrawRule& _rule) const {
    return true;
}


void Builder::addPoint(const Point& _point, const Properties& _props, const DrawRule& _rule) {
    AlfonsStyle::Parameters params = applyRule(_rule, _props);

    if (!prepareLabel(params, Label::Type::point)) { return; }

    addLabel(params, Label::Type::point, { glm::vec2(_point), glm::vec2(_point) });
}

void Builder::addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) {

    AlfonsStyle::Parameters params = applyRule(_rule, _props);

    if (!prepareLabel(params, Label::Type::line)) { return; }

    float pixel = 2.0 / m_tileSize;
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

bool Builder::prepareLabel(const AlfonsStyle::Parameters& _params, Label::Type _type) {

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
    float fontScale = _params.fontSize / FONT_SIZE;

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

    LOG("LABEL  %f %f - %f %f - %d",
        m_scratch.metrics.ascender, m_scratch.metrics.descender,
        m_scratch.bbox.x, m_scratch.bbox.y,
        m_scratch.vertices.size());

    return true;
}

void Builder::addLabel(const AlfonsStyle::Parameters& _params, Label::Type _type,
                       Label::Transform _transform) {

    int vertexOffset = m_vertices.size();
    int numVertices = m_scratch.vertices.size();

    m_labels.emplace_back(new TextLabel(_transform, _type,
                                        m_scratch.bbox, *m_mesh,
                                        { vertexOffset, numVertices },
                                        _params.labelOptions,
                                        m_scratch.metrics,
                                        m_scratch.numLines,
                                        _params.anchor,
                                        m_scratch.quadsLocalOrigin));

    m_vertices.insert(m_vertices.end(),
                      m_scratch.vertices.begin(),
                      m_scratch.vertices.end());
}

AlfonsStyle::Parameters Builder::applyRule(const DrawRule& _rule,
                                           const Properties& _props) const {

    const static std::string key_name("name");

    AlfonsStyle::Parameters p;

    std::string fontFamily, fontWeight, fontStyle, transform, align, anchor;
    glm::vec2 offset;

    _rule.get(StyleParamKey::font_family, fontFamily);
    _rule.get(StyleParamKey::font_weight, fontWeight);
    _rule.get(StyleParamKey::font_style, fontStyle);

    fontWeight = (fontWeight.size() == 0) ? "400" : fontWeight;
    fontStyle = (fontStyle.size() == 0) ? "normal" : fontStyle;
    // {
    //     if (!m_fontContext->lock()) { return p; }
    //     p.fontId = m_fontContext->addFont(fontFamily, fontWeight, fontStyle);
    //     m_fontContext->unlock();
    //     if (p.fontId < 0) { return p; }
    // }

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

    std::hash<AlfonsStyle::Parameters> hash;
    p.labelOptions.paramHash = hash(p);

    // Stroke width is normalized by the distance of the SDF spread, then scaled
    // to a char, then packed into the "alpha" channel of stroke. The .25 scaling
    // probably has to do with how the SDF is generated, but honestly I'm not sure
    // what it represents.
    //uint32_t strokeWidth = p.strokeWidth / p.blurSpread * 255. * .25;
    uint32_t strokeWidth = p.strokeWidth / 3.f * 255. * .25;
    p.stroke = (p.strokeColor & 0x00ffffff) + (strokeWidth << 24);

    return p;
}

}

std::unique_ptr<StyleBuilder> AlfonsStyle::createBuilder() const {
    return std::make_unique<Builder>(*this);
}

}
