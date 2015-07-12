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

    using Mesh = TypedMesh<PosNormColVertex>;

    class PolygonBatch : public Batch {
    public:
        PolygonBatch(const PolygonStyle& _style) : m_style(_style) {
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
        const PolygonStyle& m_style;
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void build(Batch& _batch, const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) const override;

    virtual Batch* newBatch() const override { return new PolygonBatch(*this); };

    void buildLine(PolygonBatch& _batch, const Line& _line, const Properties& _props, const StyleParams& _params) const;
    void buildPolygon(PolygonBatch& _batch, const Polygon& _polygon, const Properties& _props, const StyleParams& _params) const;

public:

    PolygonStyle(GLenum _drawMode = GL_TRIANGLES);
    PolygonStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolygonStyle() {
    }
};
