#pragma once

#include "style.h"
#include "rawVboMesh.h"
#include "glfontstash.h"
#include "tile/labels/labelContainer.h"
#include <memory>

class FontStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void prepareDataProcessing(MapTile& _tile) const override;
    virtual void finishDataProcessing(MapTile& _tile) const override;

    virtual VboMesh* newMesh() const override {
      return new RawVboMesh(m_vertexLayout, m_drawMode);
    };

public:

    FontStyle(const std::string& _fontName, std::string _name, float _fontSize, bool _sdf = false, GLenum _drawMode = GL_TRIANGLES);

    virtual void setupFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) override;
    virtual void setupTile(const std::shared_ptr<MapTile>& _tile) override;
    virtual void teardown() override;

    virtual ~FontStyle();

    /*
     * A pointer to the tile being currently processed, e.g. the tile which data is being added to
     * nullptr if no tile is being processed
     */
    static MapTile* processedTile;
    
private:

    std::string m_fontName;
    float m_fontSize;
    bool m_sdf;

};
