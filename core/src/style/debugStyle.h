#pragma once

#include "style.h"
#include "typedMesh.h"

class DebugStyle : public Style {
    
protected:
    
    struct PosColVertex {
        // Position Data
        GLfloat pos_x;
        GLfloat pos_y;
        GLfloat pos_z;
        // Color Data
        GLuint abgr;
    };
    
    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void addData(TileData& _data, MapTile& _tile, const MapProjection& _mapProjection) const override;
    
    typedef TypedMesh<PosColVertex> Mesh;
    
    virtual VboMesh* newMesh() const override {
        return nullptr;
    };

public:
    
    DebugStyle(GLenum _drawMode = GL_LINE_LOOP);
    DebugStyle(std::string _name, GLenum _drawMode = GL_LINE_LOOP);
    
    virtual ~DebugStyle() {
    }
    
};
