#pragma once

#include "style.h"
#include "labels/textLabel.h"
#include "labels/labelProperty.h"
#include "util/hash.h"
#include "gl/dynamicQuadMesh.h"

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
        TextLabelProperty::Align align = TextLabelProperty::Align::center;
        LabelProperty::Anchor anchor = LabelProperty::Anchor::center;

        float fontScale = 1;
        float lineSpacing = 0;
    };

    auto& context() const { return m_context; }

protected:

    bool m_sdf;

    std::shared_ptr<FontContext> m_context;

    UniformLocation m_uTexScaleFactor{"u_uv_scale_factor"};
    UniformLocation m_uTex{"u_tex"};
    UniformLocation m_uOrtho{"u_ortho"};
    UniformLocation m_uPass{"u_pass"};
    UniformLocation m_uMaxStrokeWidth{"u_max_stroke_width"};

    mutable std::vector<std::unique_ptr<DynamicQuadMesh<TextVertex>>> m_meshes;

public:

    TextStyle(std::string _name, std::shared_ptr<FontContext> _fontContext, bool _sdf = false,
              Blending _blendMode = Blending::overlay, GLenum _drawMode = GL_TRIANGLES);

    void constructVertexLayout() override;
    void constructShaderProgram() override;

    /* Create the LabelMeshes associated with FontContext GlyphTexture<s>
     * No GL involved, called from Tangram::update()
     */
    void onBeginUpdate() override;

    /* Upload the buffers of the text batches
     * Upload the texture atlases
     */
    void onBeginFrame() override;

    /* Performs the actual drawing of the meshes in two passes
     * - First pass if signed distance field is on, draw outlines
     * - Second pass, draw the inner glyph pixels
     */
    void onBeginDrawFrame(const View& _view, Scene& _scene) override;

    std::unique_ptr<StyleBuilder> createBuilder() const override;

    DynamicQuadMesh<TextVertex>& getMesh(size_t id) const;

    virtual size_t dynamicMeshSize() const override;

    virtual ~TextStyle() override;

    Parameters defaultUnifiedParams() const;

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
