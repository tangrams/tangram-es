#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "util/typedMesh.h"
#include "tile/labels/labels.h"
#include "tile/labels/spriteAtlas.h"

class Texture;

class SpriteStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, void* _styleParam, Properties& _props, VboMesh& _mesh, MapTile& _tile) const override;
    virtual void buildLine(Line& _line, void* _styleParam, Properties& _props, VboMesh& _mesh, MapTile& _tile) const override;
    virtual void buildPolygon(Polygon& _polygon, void* _styleParam, Properties& _props, VboMesh& _mesh, MapTile& _tile) const override;

    virtual void* parseStyleParams(const std::string& _layerNameID, const StyleParamMap& _styleParamMap) override;

    typedef TypedMesh<BufferVert> Mesh;

    virtual VboMesh* newMesh() const override {
        return new Mesh(m_vertexLayout, m_drawMode, GL_DYNAMIC_DRAW);
    };
    
    std::unique_ptr<SpriteAtlas> m_spriteAtlas;
    std::shared_ptr<Labels> m_labels;
    unsigned int m_spriteGeneration;
    
public:

    virtual void onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) override;
    virtual void onEndDrawFrame() override;
    
    void setSpriteAtlas(std::unique_ptr<SpriteAtlas> _atlas);

    SpriteStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~SpriteStyle();

};
