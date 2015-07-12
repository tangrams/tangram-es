#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "typedMesh.h"
#include "util/builders.h"

#include <mutex>




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

    using Mesh = TypedMesh<PosNormEnormColVertex>;

    class PolylineBatch : public Batch {
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
        std::shared_ptr<Mesh> m_mesh;
        private:
        const PolylineStyle& m_style;

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

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    /*
     * Parse StyleParamMap to individual style's StyleParam structure.
     */
    void parseStyleParams(const StyleParamMap& _styleParamMap, StyleParams& _styleParams) const;

    virtual void build(Batch& _batch, const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) const;

    virtual void buildLine(PolylineBatch& _batch, const Line& _line, const StyleParams& _styleParam, const Properties& _props, const MapTile& _tile) const;

    virtual Batch* newBatch() const override {
        return new PolylineBatch(*this);
    };

public:

    PolylineStyle(GLenum _drawMode = GL_TRIANGLES);
    PolylineStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolylineStyle() {
    }
};
