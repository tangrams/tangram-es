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
        std::shared_ptr<Properties> properties;
        uint32_t fill = 0xff000000;
        uint32_t strokeColor = 0xffffffff;
        float strokeWidth = 0.0f;
        float fontSize = 16.0f;
        float blurSpread = 0.0f;
        bool visible = true;
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

    virtual void buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual bool checkRule(const DrawRule& _rule) const override;

    virtual VboMesh* newMesh() const override;

    Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    /* Creates a text label and add it to the processed <TextBuffer>. */
    void addTextLabel(TextBuffer& _buffer, Label::Transform _transform, std::string _text, Label::Type _type) const;

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
            hash_combine(seed, p.visible);
            hash_combine(seed, p.wordWrap);
            hash_combine(seed, p.maxLineWidth);
            hash_combine(seed, p.transform);
            hash_combine(seed, p.align);
            hash_combine(seed, p.anchor);
            hash_combine(seed, optionsHash(p.labelOptions));
            return seed;
        }
    };
}

