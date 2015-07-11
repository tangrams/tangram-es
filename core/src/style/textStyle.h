#pragma once

#include "style.h"
#include "typedMesh.h"
#include "glfontstash.h"
#include "tile/labels/labels.h"
#include "text/fontContext.h"
#include "text/textBuffer.h"

#include <memory>

class TextBatch;

class TextStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, const StyleParamMap& _styleParamMap, Properties& _props, Batch& _batch, MapTile& _tile) const override;
    virtual void buildLine(Line& _line, const StyleParamMap& _styleParamMap, Properties& _props, Batch& _batch, MapTile& _tile) const override;
    virtual void buildPolygon(Polygon& _polygon, const StyleParamMap& _styleParamMap, Properties& _props, Batch& _batch, MapTile& _tile) const override;
    virtual void onBeginBuildTile(Batch& _batch) const override;
    virtual void onEndBuildTile(Batch& _batch) const override;


    virtual Batch* newBatch() const override;
    
    std::string m_fontName;
    float m_fontSize;
    int m_color;
    bool m_sdf;
    bool m_sdfMultisampling = true;

    std::shared_ptr<Labels> m_labels;

public:

    TextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color = 0xffffff,
              bool _sdf = false, bool _sdfMultisampling = false, GLenum _drawMode = GL_TRIANGLES);

    virtual void onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) override;
    virtual void onEndDrawFrame() override;

    virtual ~TextStyle();

    friend class TextBatch;
};
