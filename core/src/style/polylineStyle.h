#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "gl/typedMesh.h"
#include "util/builders.h"

#include <mutex>

namespace Tangram {

class PolylineStyle : public Style {

protected:

    struct Parameters {
        int32_t order = 0;
        uint32_t color = 0xffffffff;
        uint32_t outlineColor = 0xffffffff;
        float width = 1.f;
        float outlineWidth = 1.f;
        CapTypes cap = CapTypes::butt;
        CapTypes outlineCap = CapTypes::butt;
        JoinTypes join = JoinTypes::miter;
        JoinTypes outlineJoin = JoinTypes::miter;
        bool outlineOn = false;
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
    virtual void buildLine(const Line& _line, const StyleParamMap& _styleParamMap, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    Parameters parseStyleParams(const StyleParamMap& _styleParamMap) const;

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

}
