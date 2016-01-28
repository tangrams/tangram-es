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
#include "gl/renderState.h"

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

#include <bitset>


namespace alf = alfons;

namespace Tangram {

constexpr int texture_size = 256;
constexpr int max_textures = 64;

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
#define SDF_WIDTH 3

#define ANDROID_FONT_PATH "/system/fonts/"

auto vertexLayout() {
    return std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_SHORT, false, 0},
        {"a_uv", 2, GL_UNSIGNED_SHORT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_stroke", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_screenPosition", 2, GL_SHORT, false, 0},
        {"a_alpha", 1, GL_SHORT, true, 0},
        {"a_rotation", 1, GL_SHORT, false, 0},
    }));
}

struct GlyphQuad {
    struct {
        glm::i16vec2 pos;
        glm::u16vec2 uv;
    } quad[4];
    uint32_t color;
    uint32_t stroke;
    alf::AtlasID atlas;
};

// NB: just using LabelMesh here to reuse its quadIndices
// for drawing..
struct AlfonsMesh : public LabelMesh {
    using LabelMesh::LabelMesh;

    int bufferCapacity = 0;
    std::vector<Label::Vertex> m_vertices;

    void pushQuad(GlyphQuad& _quad, Label::Vertex::State& _state) {

        m_vertices.resize(m_nVertices + 4);

        for (int i = 0; i < 4; i++) {
            Label::Vertex& v = m_vertices[m_nVertices+i];
            v.pos = _quad.quad[i].pos;
            v.uv = _quad.quad[i].uv;
            v.color = _quad.color;
            v.stroke = _quad.stroke;
            v.state = _state;
        }
        m_nVertices += 4;
    }

    void myUpload() {

        const size_t maxVertices = 16384;

        if (m_nVertices == 0) { return; }

        m_vertexOffsets.clear();
        for (size_t offset = 0; offset < m_nVertices; offset += maxVertices) {
            size_t nVertices = maxVertices;
            if (offset + maxVertices > m_nVertices) {
                nVertices = m_nVertices - offset;
            }
            m_vertexOffsets.emplace_back(nVertices / 4 * 6, nVertices);
        }

        if (!checkValidity()) {
            loadQuadIndices();
            bufferCapacity = 0;
        }

        // Generate vertex buffer, if needed
        if (m_glVertexBuffer == 0) { glGenBuffers(1, &m_glVertexBuffer); }

        // Buffer vertex data
        int vertexBytes = m_nVertices * m_vertexLayout->getStride();

        RenderState::vertexBuffer(m_glVertexBuffer);

        if (vertexBytes > bufferCapacity) {
            bufferCapacity = vertexBytes;

            glBufferData(GL_ARRAY_BUFFER, vertexBytes,
                         reinterpret_cast<GLbyte*>(m_vertices.data()),
                         m_hint);
        } else {
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertexBytes,
                            reinterpret_cast<GLbyte*>(m_vertices.data()));
        }
        m_isCompiled = true;
        m_isUploaded = true;
        m_dirty = false;
    }

    bool ready() { return m_isCompiled; }

    void clear() {
        m_nVertices = 0;
        m_vertices.clear();
        m_isCompiled = false;
    }
};


struct GlyphBatch {

    GlyphBatch(std::shared_ptr<VertexLayout> _vertexLayout)
        : texture(texture_size, texture_size) {

        texData.resize(texture_size * texture_size);
        mesh = std::make_unique<AlfonsMesh>(_vertexLayout, GL_TRIANGLES);
    }

    std::vector<unsigned char> texData;
    Texture texture;
    bool dirty;
    std::unique_ptr<AlfonsMesh> mesh;

    size_t refCount = 0;
};

struct AlfonsContext : public alf::TextureCallback {
    AlfonsContext() :
        m_atlas(*this, texture_size) {

#if defined(PLATFORM_ANDROID)

        auto fontPath = systemFontPath("sans-serif", "400", "normal");
        LOG("FONT %s", fontPath.c_str());

        m_font = m_alfons.addFont(fontPath, FONT_SIZE);

        std::string fallback = "";
        int importance = 0;

        while (importance < 100) {
            fallback = systemFontFallbackPath(importance++, 400);
            if (fallback.empty()) {
                break;
            }
            if (fallback.find("UI-") != std::string::npos) {
                continue;
            }
            fontPath = ANDROID_FONT_PATH;
            fontPath += fallback;
            LOG("FALLBACK %s", fontPath.c_str());

            m_font->addFace(m_alfons.getFontFace(alf::InputSource(fontPath), FONT_SIZE));
        }
#else
        m_font = m_alfons.addFont(DEFAULT, FONT_SIZE);
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FONT_AR), FONT_SIZE));
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FONT_HE), FONT_SIZE));
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FONT_JA), FONT_SIZE));
        m_font->addFace(m_alfons.getFontFace(alf::InputSource(FALLBACK), FONT_SIZE));
#endif


        m_vertexLayout = vertexLayout();
    }

    // Synchronized on m_mutex on tile-worker threads
    void addTexture(alf::AtlasID id, uint16_t width, uint16_t height) override {
        m_batches.emplace_back(m_vertexLayout);
    }

    // Synchronized on m_mutex, called tile-worker threads
    void addGlyph(alf::AtlasID id, uint16_t gx, uint16_t gy, uint16_t gw, uint16_t gh,
                  const unsigned char* src, uint16_t pad) override {

        auto& texData = m_batches[id].texData;
        auto& texture = m_batches[id].texture;
        m_batches[id].dirty = true;

        uint16_t stride = texture_size;
        uint16_t width = texture_size;

        unsigned char* dst = &texData[(gx + pad) + (gy + pad) * stride];

        size_t pos = 0;
        for (uint16_t y = 0; y < gh; y++) {
            for (uint16_t x = 0; x < gw; x++) {
                //dst[x + (y * stride)] += std::min(src[pos++] + 30,  0xff);
                dst[x + (y * stride)] = src[pos++];
            }
        }

        dst = &texData[gx + gy * width];
        gw += pad * 2;
        gh += pad * 2;

        size_t bytes = gw * gh * sizeof(float) * 3;
        if (m_sdfBuffer.size() < bytes) {
            m_sdfBuffer.resize(bytes);
        }

        sdfBuildDistanceFieldNoAlloc(dst, width, SDF_WIDTH,
                                     dst, gw, gh, width,
                                     &m_sdfBuffer[0]);

        texture.setDirty(gy, gh);
    }

    void releaseAtlas(std::bitset<max_textures> _refs) {
        if (!_refs.any()) { return; }
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < m_batches.size(); i++) {
            if (!_refs[i]) { continue; }

            if (--m_atlasRefCount[i] == 0) {
                LOG("CLEAR ATLAS %d", i);
                m_atlas.clear(i);
                m_batches[i].texData.assign(texture_size * texture_size, 0);
            }
        }
    }
    void lockAtlas(std::bitset<max_textures> _refs) {
        if (!_refs.any()) { return; }
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < m_batches.size(); i++) {
            if (_refs[i]) { m_atlasRefCount[i]++; }
        }
    }

    alf::FontManager m_alfons;
    alf::GlyphAtlas m_atlas;

    std::shared_ptr<alf::Font> m_font;
    std::vector<unsigned char> m_sdfBuffer;
    std::vector<GlyphBatch> m_batches;
    std::shared_ptr<VertexLayout> m_vertexLayout;
    std::array<int, max_textures> m_atlasRefCount = {{0}};

    std::mutex m_mutex;
};

// Not actually used as VboMesh!
// TODO make LabelMesh an interface
// - just keeps labels and vertices for all Labels of a tile
struct LabelContainer : public LabelSet, public Mesh {

    LabelContainer(AlfonsContext& _ctx) : context(_ctx) {}

    ~LabelContainer() override {
        context.releaseAtlas(atlasRefs);
    }

    AlfonsContext& context;
    std::vector<GlyphQuad> quads;
    std::bitset<max_textures> atlasRefs;

    void draw(ShaderProgram& _shader) override {}
    size_t bufferSize() override { return 0; }

    void setLabels(std::vector<std::unique_ptr<Label>>& _labels,
                   std::vector<GlyphQuad>& _quads) {

        typedef std::vector<std::unique_ptr<Label>>::iterator iter_t;

        m_labels.reserve(_labels.size());
        m_labels.insert(m_labels.begin(),
                        std::move_iterator<iter_t>(_labels.begin()),
                        std::move_iterator<iter_t>(_labels.end()));

        quads.reserve(_quads.size());
        quads.insert(quads.begin(),
                     _quads.begin(),
                     _quads.end());

        for (auto& q : quads) {
            atlasRefs.set(q.atlas);
        }

        context.lockAtlas(atlasRefs);
    }
};


AlfonsStyle::AlfonsStyle(std::string _name, bool _sdf, Blending _blendMode, GLenum _drawMode) :
    Style(_name, _blendMode, _drawMode), m_sdf(_sdf),
    m_context(std::make_shared<AlfonsContext>()) {}

AlfonsStyle::~AlfonsStyle() {}

void AlfonsStyle::constructVertexLayout() {
    m_vertexLayout = vertexLayout();
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
    if (m_context->m_batches.empty()) { return; }

    {
        std::lock_guard<std::mutex> lock(m_context->m_mutex);
        int tex = 0;

        for (auto& batch : m_context->m_batches) {
            bool dirty = false;

            if (batch.dirty || !batch.texture.isValid()) {
                batch.dirty = false;
                dirty = true;

                auto td = reinterpret_cast<const GLuint*>(batch.texData.data());
                batch.texture.update(0, td);
            }

            batch.mesh->myUpload();

            LOG(">>> refcount:%d texture:%d/%d texid:%d upload:%d - buffersize:%d",
                m_context->m_atlasRefCount[tex], tex,
                m_context->m_batches.size(),
                batch.texture.getGlHandle(),
                dirty, batch.mesh->bufferSize());
            tex++;
        }
    }
    m_shaderProgram->setUniformf("u_uv_scale_factor",
                                 glm::vec2(1.0f / texture_size));
    m_shaderProgram->setUniformi("u_tex", 0);
    m_shaderProgram->setUniformMatrix4f("u_ortho", _view.getOrthoViewportMatrix());

    Style::onBeginDrawFrame(_view, _scene, 1);
}

void AlfonsStyle::onEndDrawFrame() {
    if (m_context->m_batches.empty()) { return; }

    std::lock_guard<std::mutex> lock(m_context->m_mutex);

    if (m_sdf) {
        m_shaderProgram->setUniformi("u_pass", 1);
        for (auto& gt : m_context->m_batches) {
            if (gt.mesh->ready()) {
                gt.texture.bind(0);
                gt.mesh->draw(*m_shaderProgram);
            }
        }
        m_shaderProgram->setUniformi("u_pass", 0);
    }

    for (auto& gt : m_context->m_batches) {
        if (gt.mesh->ready()) {

            gt.texture.bind(0);
            gt.mesh->draw(*m_shaderProgram);

            // FIXME - resets for next frame..
            // should be done only when buffers are changing
            gt.mesh->clear();
        }
    }
}

namespace {

struct Builder : public StyleBuilder {

    const AlfonsStyle& m_style;

    float m_tileSize;

    alf::TextShaper m_shaper;
    alf::TextBatch m_batch;

    bool m_sdf;
    float m_pixelScale = 1;

    std::unique_ptr<LabelContainer> m_mesh;
    std::vector<std::unique_ptr<Label>> m_labels;

    struct ScratchBuffer : public alf::MeshCallback {
        std::vector<GlyphQuad> quads;

        // label width and height
        glm::vec2 bbox;
        glm::vec2 quadsLocalOrigin;
        int numLines;
        FontContext::FontMetrics metrics;
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
        m_labels.clear();
        m_scratch.clear();

        m_mesh = std::make_unique<LabelContainer>(*m_style.context());
    }

    virtual std::unique_ptr<Mesh> build() override {
        if (!m_labels.empty()) {
            m_mesh->setLabels(m_labels, m_scratch.quads);
        }
        m_labels.clear();
        m_scratch.clear();

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

void Builder::addLabel(const AlfonsStyle::Parameters& _params, Label::Type _type,
                       Label::Transform _transform) {

    int numQuads = m_scratch.numQuads;
    int quadOffset = m_scratch.quads.size() - numQuads;

    m_labels.emplace_back(new AlfonsLabel(_transform, _type,
                                          m_scratch.bbox, *m_mesh,
                                          { quadOffset, numQuads },
                                          _params.labelOptions,
                                          m_scratch.metrics,
                                          m_scratch.numLines,
                                          _params.anchor,
                                          m_scratch.quadsLocalOrigin));
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
        // Default to hash on all used layer names ('draw.key' in JS version)
        for (auto* name : _rule.getLayerNames()) {
            hash_combine(repeatGroupHash, name);
        }
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

    return p;
}

}

std::unique_ptr<StyleBuilder> AlfonsStyle::createBuilder() const {
    return std::make_unique<Builder>(*this);
}


using namespace LabelProperty;

AlfonsLabel::AlfonsLabel(Label::Transform _transform, Type _type, glm::vec2 _dim,
                     LabelContainer& _mesh, Range _vertexRange,
                     Label::Options _options, FontContext::FontMetrics _metrics,
                     int _nLines, Anchor _anchor, glm::vec2 _quadsLocalOrigin)
    : Label(_transform, _dim, _type, _vertexRange, _options),
      m_metrics(_metrics),
      m_nLines(_nLines),
      m_mesh(_mesh),
      m_quadLocalOrigin(_quadsLocalOrigin) {

    if (m_type == Type::point) {
        glm::vec2 halfDim = m_dim * 0.5f;
        switch(_anchor) {
            case Anchor::left: m_anchor.x -= halfDim.x; break;
            case Anchor::right: m_anchor.x += halfDim.x; break;
            case Anchor::top: m_anchor.y -= halfDim.y; break;
            case Anchor::bottom: m_anchor.y += halfDim.y; break;
            case Anchor::bottom_left: m_anchor += glm::vec2(-halfDim.x, halfDim.y); break;
            case Anchor::bottom_right: m_anchor += halfDim; break;
            case Anchor::top_left: m_anchor -= halfDim; break;
            case Anchor::top_right: m_anchor += glm::vec2(halfDim.x, -halfDim.y); break;
            case Anchor::center: break;
        }
    }
}

void AlfonsLabel::updateBBoxes(float _zoomFract) {
    glm::vec2 obbCenter;

    if (m_type == Type::line) {
        m_xAxis = glm::vec2(cos(m_transform.state.rotation), sin(m_transform.state.rotation));
        m_yAxis = glm::vec2(-m_xAxis.y, m_xAxis.x);
    }

    obbCenter = m_transform.state.screenPos;
    // move forward on line by half the text length
    obbCenter += m_xAxis * m_dim.x * 0.5f;
    // move down on the perpendicular to estimated font baseline
    obbCenter += m_yAxis * m_dim.y * 0.5f;
    // ajdust with local origin of the quads
    obbCenter += m_yAxis * m_quadLocalOrigin.y;
    obbCenter += m_xAxis * m_quadLocalOrigin.x;

    m_obb = OBB(obbCenter.x, obbCenter.y, m_transform.state.rotation,
                m_dim.x + m_options.buffer, m_dim.y + m_options.buffer);
    m_aabb = m_obb.getExtent();
}

void AlfonsLabel::align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) {

    switch (m_type) {
        case Type::debug:
        case Type::point:
            // modify position set by updateScreenTransform()
            _screenPosition.x -= m_dim.x * 0.5f + m_quadLocalOrigin.x;
            _screenPosition.y -= m_metrics.descender;

            if (m_nLines > 1) {
                _screenPosition.y -= m_dim.y * 0.5f;
                _screenPosition.y += (m_metrics.lineHeight + m_metrics.descender);
            }
            _screenPosition += m_anchor;
            break;
        case Type::line: {
            // anchor at line center
            _screenPosition = (_ap1 + _ap2) * 0.5f;

            // move back by half the length (so that text will be drawn centered)
            glm::vec2 direction = glm::normalize(_ap1 - _ap2);
            _screenPosition += direction * m_dim.x * 0.5f;
            _screenPosition += m_yAxis * (m_dim.y * 0.5f + m_metrics.descender);
            break;
        }
    }
}

void AlfonsLabel::pushTransform() {
    // if (!m_dirty) { return; }
    // m_dirty = false;
    if (!visibleState()) { return; }

    auto state = m_transform.state.vertex();

    auto it = m_mesh.quads.begin() + m_vertexRange.start;
    auto end = it + m_vertexRange.length;

    for (; it != end; ++it) {
        m_mesh.context.m_batches[it->atlas].mesh->pushQuad(*it, state);
    }
}

}
