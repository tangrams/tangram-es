#pragma once

#include "style.h"
#include "typedMesh.h"

#include <mutex>

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
    virtual void buildPoint(Point& _point, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void* parseStyleParams(const StyleParamMap& _styleParamMap) const override;

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
