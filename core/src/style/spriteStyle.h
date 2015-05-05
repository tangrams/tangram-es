#pragma once

#include "style.h"
#include "util/typedMesh.h"

class SpriteStyle : public Style {
    
protected:
    
    struct PosUVVertex {
        // Position Data
        GLfloat pos_x;
        GLfloat pos_y;
        GLfloat pos_z;
        // UV Data
        GLfloat u;
        GLfloat v;
    };
    
    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) const override;

    typedef TypedMesh<PosUVVertex> Mesh;
    
    virtual VboMesh* newMesh() const override {
        return nullptr;
    };
    
    std::shared_ptr<Texture> m_texture;
    
public:
    
    virtual void setupFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) override;
    
    SpriteStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);
    
    virtual ~SpriteStyle();
    
};
