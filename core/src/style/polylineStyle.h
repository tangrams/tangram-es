#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "typedMesh.h"
#include "util/builders.h"

#include <mutex>

class PolylineBatch;

class PolylineStyle : public Style {

protected:
    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual StyleBatch* newBatch() const override;

public:

    PolylineStyle(GLenum _drawMode = GL_TRIANGLES);
    PolylineStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolylineStyle() {}

    friend class PolylineBatch;
};


class PolylineBatch : public StyleBatch {

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

    using Mesh = TypedMesh<PosNormEnormColVertex>;

public:
    PolylineBatch(const PolylineStyle& _style) : m_style(_style) {
        m_mesh = std::make_shared<Mesh>(_style.m_vertexLayout, _style.m_drawMode);
    }

    virtual void draw(const View& _view) override {
        m_mesh->draw(m_style.getShaderProgram());
    };
    virtual void update(const glm::mat4& mvp, const View& _view, float _dt) override {};
    virtual void prepare() override {};

    virtual bool compile() {
        if (m_mesh->numVertices() > 0) {
            m_mesh->compileVertexBuffer();
            return true;
        }
        return false;
    };

    virtual void add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) override;

    void buildLine(const Line& _line, const Properties& _props, const StyleParams& _styleParam, const MapTile& _tile);

    std::shared_ptr<Mesh> m_mesh;
private:
    const PolylineStyle& m_style;

};
