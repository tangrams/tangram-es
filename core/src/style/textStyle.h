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

    Parameters parseRule(const DrawRule& _rule) const;

    virtual VboMesh* newMesh() const override {
        return new TextBuffer(m_vertexLayout);
    };

    /*
     * Creates a text label and add it to the processed <TextBuffer>.
     */
    void addTextLabel(TextBuffer& _buffer, Label::Transform _transform, std::string _text, Label::Type _type) const;

    bool m_sdf;
    bool m_sdfMultisampling = true;

public:

    bool isOpaque() const override { return false; }

    TextStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES, bool _sdf = false, bool _sdfMultisampling = false);

    virtual void onBeginDrawFrame(const View& _view, const Scene& _scene) override;

    virtual ~TextStyle();

};

}
