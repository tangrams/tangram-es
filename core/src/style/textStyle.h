#pragma once

#include "style.h"
#include "labels/label.h"
#include "style/labelProperty.h"
#include "util/hash.h"

#include <memory>

namespace alfons { class Font; }
namespace Tangram {

class FontContext;
struct Properties;
typedef int FontID;

class AlfonsContext;
class LabelContainer;
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

class DebugTextStyle : public TextStyle {

public:
    DebugTextStyle(std::string _name, bool _sdf = false) : TextStyle(_name, _sdf) {}

    std::unique_ptr<StyleBuilder> createBuilder() const override;

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
