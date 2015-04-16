#pragma once

#include "style.h"
#include "typedMesh.h"

class PolylineStyle : public Style {
    
protected:
    
    struct PosNormEnormColVertex {
        //Position Data
        GLfloat pos_x;
        GLfloat pos_y;
        GLfloat pos_z;
        // Extrude Normals Data
        GLfloat texcoord_x;
        GLfloat texcoord_y;
        // Extrude Normals Data
        GLfloat enorm_x;
        GLfloat enorm_y;
        GLfloat ewidth;
        // Color Data
        GLuint abgr;
        // Layer Data
        GLfloat layer;
    };
    
    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;

    typedef TypedMesh<PosNormEnormColVertex> Mesh;
    
    virtual VboMesh* newMesh() const override {
        return new Mesh(m_vertexLayout, m_drawMode);
    };

public:
    
    PolylineStyle(GLenum _drawMode = GL_TRIANGLES);
    PolylineStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolylineStyle() {
    }
};
