#pragma once

#include "style.h"
#include "typedMesh.h"

class PolylineStyle : public Style {
    
protected:
    
    struct StyleParams {
        int32_t order = 0;
        uint32_t color = 0xffffffff;
        float width = 1.f;
        CapTypes cap = CapTypes::BUTT;
        JoinTypes join = JoinTypes::MITER;
        float outlineWidth = 1.f;
        uint32_t outlineColor = 0xffffffff;
        bool outlineOn = false;
        CapTypes outlineCap = CapTypes::BUTT;
        JoinTypes outlineJoin = JoinTypes::MITER;
    };
    
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
    virtual void buildPoint(Point& _point, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void* parseStyleParams(StyleParamMap& _styleParamMap) const override;

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
