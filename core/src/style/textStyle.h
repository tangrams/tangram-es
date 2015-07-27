#pragma once

#include "style.h"
#include "text/fontContext.h"
#include "text/textBuffer.h"

#include <memory>

namespace Tangram {

class TextStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;

    virtual void buildPoint(Point& _point, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildLine(Line& _line, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildPolygon(Polygon& _polygon, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void onBeginBuildTile(VboMesh& _mesh) const override;
    virtual void onEndBuildTile(VboMesh& _mesh) const override;

    virtual VboMesh* newMesh() const override {
        return new TextBuffer(m_vertexLayout);
    };

    /*
     * Creates a text label and add it to the processed <TextBuffer>.
     */
    void addTextLabel(TextBuffer& _buffer, Label::Transform _transform, std::string _text, Label::Type _type) const;

    std::string m_fontName;
    float m_fontSize;
    int m_color;
    bool m_sdf;
    bool m_sdfMultisampling = true;

public:

    bool isOpaque() const override { return false; }

    TextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color = 0xffffff,
              bool _sdf = false, bool _sdfMultisampling = false, GLenum _drawMode = GL_TRIANGLES);

    virtual void onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) override;

    virtual ~TextStyle();

};

}
