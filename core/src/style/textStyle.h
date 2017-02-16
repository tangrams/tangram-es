#pragma once

#include "gl/dynamicQuadMesh.h"
#include "style/style.h"
#include "labels/labelProperty.h"
#include "labels/textLabel.h"
#include "util/hash.h"

#include <memory>
#include <vector>
#include <string>

namespace alfons { class Font; }

namespace Tangram {

class FontContext;
struct Properties;

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
        Label::Options labelOptions;
        bool wordWrap = true;
        uint32_t maxLineWidth = 15;

        TextLabelProperty::Transform transform = TextLabelProperty::Transform::none;
        TextLabelProperty::Align align = TextLabelProperty::Align::none;

        float fontScale = 1;
        float lineSpacing = 0;

        bool hasComplexShaping = false;
    };

    auto& context() const { return m_context; }

protected:

    bool m_sdf;

    std::shared_ptr<FontContext> m_context;

    struct UniformBlock {
        UniformLocation uTexScaleFactor{"u_uv_scale_factor"};
        UniformLocation uTex{"u_tex"};
        UniformLocation uOrtho{"u_ortho"};
        UniformLocation uPass{"u_pass"};
        UniformLocation uMaxStrokeWidth{"u_max_stroke_width"};
    } m_mainUniforms, m_selectionUniforms;

    mutable std::vector<std::unique_ptr<DynamicQuadMesh<TextVertex>>> m_meshes;

public:

    TextStyle(std::string _name, std::shared_ptr<FontContext> _fontContext, bool _sdf = false,
              Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_TRIANGLES, bool _selection = true);

    void constructVertexLayout() override;
    void constructShaderProgram() override;

    /* Create the LabelMeshes associated with FontContext GlyphTexture<s>
     * No GL involved, called from Tangram::update()
     */
    virtual void onBeginUpdate() override;

    /* Upload the buffers of the text batches
     * Upload the texture atlases
     */
    virtual void onBeginFrame(RenderState& rs) override;

    /* Performs the actual drawing of the meshes in two passes
     * - First pass if signed distance field is on, draw outlines
     * - Second pass, draw the inner glyph pixels
     */
    virtual void onBeginDrawFrame(RenderState& rs, const View& _view, Scene& _scene) override;

    virtual void onBeginDrawSelectionFrame(RenderState& rs, const View& _view, Scene& _scene) override;

    virtual void draw(RenderState& rs, const Tile& _tile) override {}

    virtual void draw(RenderState& rs, const Marker& _marker) override {}

    std::unique_ptr<StyleBuilder> createBuilder() const override;

    DynamicQuadMesh<TextVertex>& getMesh(size_t id) const;

    auto& getMeshes() const { return m_meshes; }

    virtual size_t dynamicMeshSize() const override;

    virtual ~TextStyle() override;

private:

    const std::string& applyTextSource(const Parameters& _parameters, const Properties& _props) const;

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
            hash_combine(seed, p.fill);
            hash_combine(seed, p.strokeColor);
            hash_combine(seed, p.strokeWidth);
            hash_combine(seed, p.fontSize);
            hash_combine(seed, p.wordWrap);
            hash_combine(seed, p.maxLineWidth);
            hash_combine(seed, int(p.transform));
            hash_combine(seed, int(p.align));
            hash_combine(seed, optionsHash(p.labelOptions));
            return seed;
        }
    };
}
