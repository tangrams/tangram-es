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
        uint32_t order = 0;
        uint32_t color = 0xff00ffff;
        uint32_t outlineColor = 0xff00ffff;
        CapTypes cap = CapTypes::butt;
        CapTypes outlineCap = CapTypes::butt;
        JoinTypes join = JoinTypes::miter;
        JoinTypes outlineJoin = JoinTypes::miter;
        bool outlineOn = false;
        glm::vec2 extrude;
    };

    struct PolylineVertex {
        glm::vec3 pos;
        glm::vec2 texcoord;
        glm::vec4 extrude;
        GLuint abgr;
        GLfloat layer;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildPolygon(const Polygon& _poly, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    Parameters parseRule(const DrawRule& _rule) const;

    typedef TypedMesh<PolylineVertex> Mesh;

    virtual VboMesh* newMesh() const override {
        return new Mesh(m_vertexLayout, m_drawMode);
    };

public:

    PolylineStyle(std::string _name, Blending _blendMode = Blending::none, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolylineStyle() {
    }
};

}
