#pragma once

#include "style.h"
#include "gl/typedMesh.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include <mutex>

namespace Tangram {

class PolygonStyle : public Style {

protected:

    struct StyleParams {
        int32_t order = 0;
        uint32_t color = 0xffffffff;
    };

    struct PosNormColVertex {
        // Position Data
        glm::vec3 pos;
        // Normal Data
        glm::vec3 norm;
        // UV Data
        glm::vec2 texcoord;
        // Color Data
        GLuint abgr;
        // Layer Data
        GLfloat layer;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildLine(const Line& _line, const StyleParamMap& _styleParamMap, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    virtual void buildPolygon(const Polygon& _polygon, const StyleParamMap& _styleParamMap, const Properties& _props, VboMesh& _mesh, Tile& _tile) const override;
    /*
     * Parse StyleParamMap to individual style's StyleParam structure.
     */
    void parseStyleParams(const StyleParamMap& _styleParamMap, StyleParams& _styleParams) const;

    typedef TypedMesh<PosNormColVertex> Mesh;

    virtual VboMesh* newMesh() const override {
        return new Mesh(m_vertexLayout, m_drawMode);
    };

public:

    PolygonStyle(GLenum _drawMode = GL_TRIANGLES);
    PolygonStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolygonStyle() {
    }
};

}
