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
        uint32_t order = 0;
        uint32_t color = 0xff00ffff;
        glm::vec2 extrude;
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
    virtual void buildPolygon(const Polygon& _polygon, const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    virtual void buildMesh(const std::vector<uint16_t>& indices, const std::vector<Point>& points,
                           const DrawRule& _rule, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    Parameters parseRule(const DrawRule& _rule) const;

    typedef TypedMesh<PolygonVertex> Mesh;

    virtual VboMesh* newMesh() const override {
        return new Mesh(m_vertexLayout, m_drawMode);
    };

    void addVertex(glm::vec3 p, glm::vec3 n, GLuint abgr, float layer,
                   std::vector<uint16_t>& indices,
                   std::vector<PolygonVertex>& vertices) const;

public:

    PolygonStyle(std::string _name, Blending _blendMode = Blending::none, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolygonStyle() {
    }
};

}
