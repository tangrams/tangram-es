#pragma once

#include "style.h"
#include "typedMesh.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include <mutex>

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
    virtual void buildPoint(Point& _point, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, void* _styleParam, Properties& _props, VboMesh& _mesh) const override;
    virtual void* parseStyleParams(const std::string& _layerNameID, const StyleParamMap& _styleParamMap) override;

    typedef TypedMesh<PosNormColVertex> Mesh;

    virtual VboMesh* newMesh() const override {
        return new Mesh(m_vertexLayout, m_drawMode);
    };

    std::unordered_map<std::string, StyleParams*> m_styleParamCache;
    std::mutex m_cacheMutex;

public:

    PolygonStyle(GLenum _drawMode = GL_TRIANGLES);
    PolygonStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolygonStyle() {
        for(auto& styleParam : m_styleParamCache) {
            delete styleParam.second;
        }
        m_styleParamCache.clear();
    }
};
