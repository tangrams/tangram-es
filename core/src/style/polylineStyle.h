#pragma once

#include "style.h"
#include "typedMesh.h"

class PolylineStyle : public Style {
    
protected:
    
    struct PosNormEnormColVertex {
        //Position Data
        glm::vec3 pos;
        // UV Data
        glm::vec2 texcoord;
        // Extrude Normals Data
        glm::vec2 enorm;
        GLfloat ewidth;
        // Color Data
        GLuint abgr;
        // Layer Data
        GLfloat layer;
    };
    
    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, StyleParams& _params, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, StyleParams& _params, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, StyleParams& _params, Properties& _props, VboMesh& _mesh) const override;

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
