#pragma once

#include "style.h"
#include "text/textBuffer.h"

#include <memory>

namespace Tangram {

class TextStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;

    virtual void buildPoint(const Point& _point, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    Parameters applyRule(const DrawRule& _rule, const Properties& _props) const;

    virtual VboMesh* newMesh() const override {
        return new TextBuffer(m_vertexLayout);
    };

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

    virtual void onBeginDrawFrame(const View& _view, Scene& _scene) override;

    virtual ~TextStyle();

private:

    const std::string& applyTextSource(const Parameters& _parameters, const Properties& _props) const;
};

}
