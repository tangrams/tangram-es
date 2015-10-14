#pragma once

#include "style.h"
#include "labels/label.h"

#include <memory>

namespace Tangram {

class TextBuffer;
class FontContext;
struct Properties;
typedef int FontID;

enum class TextTransform {
    none,
    capitalize,
    uppercase,
    lowercase,
};

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
        float fontSize = 12.0f;
        float blurSpread = 0.0f;
        TextTransform transform = TextTransform::none;
        bool visible = true;
        Label::Options labelOptions;

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

    virtual void onBeginDrawMesh(VboMesh& _mesh) const override;

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
