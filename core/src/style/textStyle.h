#pragma once

#include "style.h"
#include "labels/label.h"
#include "labelProperty.h"
#include "util/hash.h"

#include <memory>

namespace Tangram {

class TextBuffer;
class FontContext;
struct Properties;
typedef int FontID;

class TextStyle : public Style {

public:

    struct Parameters {
        FontID fontId = -1;
        std::string text = "";
        bool interactive = false;
        uint32_t fill = 0xff000000;
        uint32_t strokeColor = 0xffffffff;
        float strokeWidth = 0.0f;
        float fontSize = 16.0f;
        float blurSpread = 0.0f;
        Label::Options labelOptions;
        bool wordWrap = true;
        unsigned int maxLineWidth = 15;

        TextLabelProperty::Transform transform = TextLabelProperty::Transform::none;
        TextLabelProperty::Align align = TextLabelProperty::Align::center;
        LabelProperty::Anchor anchor = LabelProperty::Anchor::center;

        bool isValid() {
            return fontSize > 0.f && !text.empty();
        }
    };

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;

    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;

    bool m_sdf;
    bool m_sdfMultisampling;

    std::shared_ptr<FontContext> m_fontContext;

public:

    TextStyle(std::string _name, std::shared_ptr<FontContext> _fontContext,
              bool _sdf = false, bool _sdfMultisampling = false,
              Blending _blendMode = Blending::overlay,
              GLenum _drawMode = GL_TRIANGLES);

    virtual void onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit = 0) override;

    virtual ~TextStyle();

    bool useSDF() const { return m_sdf; }
    auto& fontContext() const { return *m_fontContext; }

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
            hash_combine(seed, p.fontId);
            hash_combine(seed, p.text);
            hash_combine(seed, p.interactive);
            hash_combine(seed, p.fill);
            hash_combine(seed, p.strokeColor);
            hash_combine(seed, p.strokeWidth);
            hash_combine(seed, p.fontSize);
            hash_combine(seed, p.wordWrap);
            hash_combine(seed, p.maxLineWidth);
            hash_combine(seed, (int)p.transform);
            hash_combine(seed, (int)p.align);
            hash_combine(seed, (int)p.anchor);
            hash_combine(seed, optionsHash(p.labelOptions));
            return seed;
        }
    };
}

