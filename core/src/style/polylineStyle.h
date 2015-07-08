#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "typedMesh.h"
#include "util/builders.h"

#include <mutex>

class PolylineStyle : public Style {

protected:

    struct StyleParams {
        int32_t order = 0;
        uint32_t color = 0xffffffff;
        float width = 1.f;
        CapTypes cap = CapTypes::butt;
        JoinTypes join = JoinTypes::miter;
        float outlineWidth = 1.f;
        uint32_t outlineColor = 0xffffffff;
        bool outlineOn = false;
        CapTypes outlineCap = CapTypes::butt;
        JoinTypes outlineJoin = JoinTypes::miter;
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
    virtual void* parseStyleParams(const std::string& _layerNameID, const StyleParamMap& _styleParamMap) override;

    typedef TypedMesh<PosNormEnormColVertex> Mesh;

    virtual VboMesh* newMesh() const override {
        return new Mesh(m_vertexLayout, m_drawMode);
    };

    std::unordered_map<std::string, StyleParams*> m_styleParamCache;
    std::mutex m_cacheMutex;

public:

    PolylineStyle(GLenum _drawMode = GL_TRIANGLES);
    PolylineStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolylineStyle() {
        for(auto& styleParam : m_styleParamCache) {
            delete styleParam.second;
        }
        m_styleParamCache.clear();
    }
};
