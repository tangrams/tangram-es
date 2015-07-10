#pragma once

#include "style.h"
#include "typedMesh.h"
#include "glfontstash.h"
#include "tile/labels/labels.h"
#include <memory>

class TextStyle : public Style {

protected:

    struct TextVert {
        glm::vec2 pos;
        glm::vec2 uvs;
        glm::vec2 screenPos;
        float alpha;
        float rotation;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, void* _styleParams, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, void* _styleParams, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, void* _styleParams, Properties& _props, VboMesh& _mesh) const override;
    virtual void onBeginBuildTile(MapTile& _tile) const override;
    virtual void onEndBuildTile(MapTile& _tile, std::shared_ptr<VboMesh> _mesh) const override;
    
    virtual void* parseStyleParams(const std::string& _layerNameID, const StyleParamMap& _styleParamMap) override;

    typedef TypedMesh<TextVert> Mesh;

    virtual VboMesh* newMesh() const override {
        return new Mesh(m_vertexLayout, m_drawMode, GL_DYNAMIC_DRAW);
    };

    std::string m_fontName;
    float m_fontSize;
    int m_color;
    bool m_sdf;
    bool m_sdfMultisampling = true;
    
    std::shared_ptr<Labels> m_labels;
    
    void addVertices(TextBuffer& _buffer, VboMesh& _mesh) const;

public:

    TextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color = 0xffffff,
              bool _sdf = false, bool _sdfMultisampling = false, GLenum _drawMode = GL_TRIANGLES);

    virtual void onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) override;

    virtual ~TextStyle();

    /*
     * A pointer to the tile being currently processed, e.g. the tile which data is being added to
     * nullptr if no tile is being processed
     */
    static MapTile* s_processedTile;

};
