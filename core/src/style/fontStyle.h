#pragma once

#include "style.h"
#include "glfontstash.h"
#include "tile/labels/labelContainer.h"
#include <memory>

class FontStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) override;
    virtual void buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) override;
    virtual void buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) override;
    virtual void prepareDataProcessing(MapTile& _tile) override;
    virtual void finishDataProcessing(MapTile& _tile) override;

public:

    FontStyle(const std::string& _fontName, std::string _name, float _fontSize, bool _sdf = false, GLenum _drawMode = GL_TRIANGLES);

    virtual void setupFrame(const std::shared_ptr<View>& _view) override;
    virtual void teardown() override;

    virtual ~FontStyle();

private:

    std::string m_fontName;
    float m_fontSize;
    bool m_sdf;

};
