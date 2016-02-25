#pragma once

#include "style.h"
#include "labels/textLabel.h"
#include "style/labelProperty.h"
#include "util/hash.h"

#include "alfons/alfons.h"
#include "alfons/textBatch.h"
#include "alfons/textShaper.h"

#include <memory>

namespace alfons {
    class Font;
    struct AtlasGlyph;
    struct Quad;
    struct Rect;
}

namespace Tangram {

class FontContext;
struct Properties;
typedef int FontID;

class AlfonsContext;
class LabelContainer;
struct GlyphQuad;
struct TextMesh;

class TextStyle : public Style {

public:

    struct Parameters {
        std::shared_ptr<alfons::Font> font;
        std::string text = "";
        bool interactive = false;
        uint32_t fill = 0xff000000;
        uint32_t strokeColor = 0xffffffff;
        float strokeWidth = 0.0f;
        float fontSize = 16.0f;
        float blurSpread = 0.0f;
        Label::Options labelOptions;
        bool wordWrap = true;
        uint32_t maxLineWidth = 15;

        TextLabelProperty::Transform transform = TextLabelProperty::Transform::none;
        TextLabelProperty::Align align = TextLabelProperty::Align::center;
        LabelProperty::Anchor anchor = LabelProperty::Anchor::center;
    };

    auto& context() const { return m_context; }

    void onBeginFrame() override;

protected:

    void constructVertexLayout() override;
    void constructShaderProgram() override;

    std::unique_ptr<StyleBuilder> createBuilder() const override;

    bool m_sdf;

    std::shared_ptr<AlfonsContext> m_context;

    UniformLocation m_uTexScaleFactor{"u_uv_scale_factor"};
    UniformLocation m_uTex{"u_tex"};
    UniformLocation m_uOrtho{"u_ortho"};
    UniformLocation m_uPass{"u_pass"};
    UniformLocation m_uMaxStrokeWidth{"u_max_stroke_width"};

    mutable std::vector<std::unique_ptr<LabelMesh>> m_meshes;

public:

    TextStyle(std::string _name, bool _sdf = false,
              Blending _blendMode = Blending::overlay,
              GLenum _drawMode = GL_TRIANGLES);

    void onBeginDrawFrame(const View& _view, Scene& _scene) override;
    void onEndDrawFrame() override;
    void onBeginUpdate() override;

    LabelMesh& mesh(size_t id) const { return *m_meshes[id]; }

    virtual ~TextStyle() override;

private:

    const std::string& applyTextSource(const Parameters& _parameters, const Properties& _props) const;
};

class TextStyleBuilder : public StyleBuilder {

public:

    TextStyleBuilder(const TextStyle& _style);

    const Style& style() const override { return m_style; }

    // StyleBuilder interface
    void addPolygon(const Polygon& _polygon, const Properties& _props, const DrawRule& _rule) override;
    void addLine(const Line& _line, const Properties& _props, const DrawRule& _rule) override;
    void addPoint(const Point& _line, const Properties& _props, const DrawRule& _rule) override;
    bool checkRule(const DrawRule& _rule) const override;

    virtual void setup(const Tile& _tile) override;

    virtual std::unique_ptr<StyledMesh> build() override;

    TextStyle::Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    bool prepareLabel(TextStyle::Parameters& _params, Label::Type _type);
    void addLabel(const TextStyle::Parameters& _params, Label::Type _type,
                  Label::Transform _transform);

    std::string applyTextTransform(const TextStyle::Parameters& _params, const std::string& _string);

protected:

    struct ScratchBuffer : public alfons::MeshCallback {
        void drawGlyph(const alfons::Quad& q, const alfons::AtlasGlyph& altasGlyph) override {}
        void drawGlyph(const alfons::Rect& q, const alfons::AtlasGlyph& atlasGlyph) override;

        void reset();
        void clear();

        std::vector<GlyphQuad> quads;
        std::vector<std::unique_ptr<Label>> labels;

        // label width and height
        glm::vec2 bbox;
        glm::vec2 quadsLocalOrigin;
        int numLines;
        FontMetrics metrics;
        int numQuads;

        uint32_t fill;
        uint32_t stroke;
        uint8_t fontScale;

        float yMin, xMin;
    };

    const TextStyle& m_style;

    float m_tileSize;

    alfons::TextShaper m_shaper;
    alfons::TextBatch m_batch;

    bool m_sdf;
    float m_pixelScale = 1;

    ScratchBuffer m_scratch;

    std::unique_ptr<TextLabels> m_textLabels;

};

}

namespace std {
    template <>
    struct hash<Tangram::TextStyle::Parameters> {
        size_t operator() (const Tangram::TextStyle::Parameters& p) const {
            std::hash<Tangram::Label::Options> optionsHash;
            std::size_t seed = 0;
            // TODO
            //hash_combine(seed, p.fontId);
            hash_combine(seed, p.text);
            hash_combine(seed, p.interactive);
            hash_combine(seed, p.fill);
            hash_combine(seed, p.strokeColor);
            hash_combine(seed, p.strokeWidth);
            hash_combine(seed, p.fontSize);
            hash_combine(seed, p.wordWrap);
            hash_combine(seed, p.maxLineWidth);
            hash_combine(seed, int(p.transform));
            hash_combine(seed, int(p.align));
            hash_combine(seed, int(p.anchor));
            hash_combine(seed, optionsHash(p.labelOptions));
            return seed;
        }
    };
}
