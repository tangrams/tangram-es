#pragma once

#include "style.h"
#include "labels/label.h"
#include "labelProperty.h"
#include "util/hash.h"

#include <memory>

namespace Tangram {

class FontContext;
struct Properties;
typedef int FontID;

struct AlfonsContext;

class AlfonsStyle : public Style {

public:

    struct Parameters {
        FontID fontId = -1;
        std::string text = "";
        bool interactive = false;
        std::shared_ptr<Properties> properties;
        uint32_t fill = 0xff000000;
        uint32_t stroke = 0xff000000;
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

    };

    auto& context() const { return m_context; }

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;

    virtual std::unique_ptr<StyleBuilder> createBuilder() const override;

    bool m_sdf;

    std::shared_ptr<AlfonsContext> m_context;
public:

    AlfonsStyle(std::string _name, bool _sdf = false,
              Blending _blendMode = Blending::overlay,
              GLenum _drawMode = GL_TRIANGLES);

    virtual void onBeginDrawFrame(const View& _view, Scene& _scene, int _textureUnit = 0) override;

    virtual ~AlfonsStyle();

private:

    const std::string& applyTextSource(const Parameters& _parameters, const Properties& _props) const;
};

}

namespace std {
    template <>
    struct hash<Tangram::AlfonsStyle::Parameters> {
        size_t operator() (const Tangram::AlfonsStyle::Parameters& p) const {
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
            hash_combine(seed, int(p.transform));
            hash_combine(seed, int(p.align));
            hash_combine(seed, int(p.anchor));
            hash_combine(seed, optionsHash(p.labelOptions));
            return seed;
        }
    };
}
