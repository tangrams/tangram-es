#pragma once

#include "style.h"
#include "gl/typedMesh.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include <mutex>
#include <tuple>

namespace Tangram {

class PolygonStyle : public Style {

protected:

    struct Parameters {
        int32_t order = 0;
        uint32_t color = 0xff00ffff;
        std::pair<float, float> extrude;
    };

    struct PolygonVertex {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec2 texcoord;
        GLuint abgr;
        GLfloat layer;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildLine(const Line& _line, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    Parameters parseRule(const DrawRule& _rule) const;

    typedef TypedMesh<PolygonVertex> Mesh;

    virtual VboMesh* newMesh() const override {
        return new Mesh(m_vertexLayout, m_drawMode);
    };

public:

    PolygonStyle(std::string _name, Blending _blendMode = Blending::none, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolygonStyle() {
    }
};

}
