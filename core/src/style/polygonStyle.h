#pragma once

#include "style.h"

class PolygonStyle : public Style {
    
protected:
    
    struct PosNormColVertex {
        // Position Data
        GLfloat pos_x;
        GLfloat pos_y;
        GLfloat pos_z;
        // Normal Data
        GLfloat norm_x;
        GLfloat norm_y;
        GLfloat norm_z;
        // UV Data
        GLfloat texcoord_x;
        GLfloat texcoord_y;
        // Color Data
        GLuint abgr;
        // Layer Data
        GLfloat layer;
    };

    class Mesh : public VboMesh {
       public:
        Mesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
            : VboMesh(_vertexLayout, _drawMode){};

        void addVertices(std::vector<PosNormColVertex>&& _vertices,
                         std::vector<int>&& _indices) {
            vertices.push_back(_vertices);
            indices.push_back(_indices);

            m_nVertices += _vertices.size();
            m_nIndices += _indices.size();
        }

       protected:
        std::vector<std::vector<PosNormColVertex>> vertices;
        std::vector<std::vector<int>> indices;

        virtual ByteBuffers compileVertexBuffer() override {
            return compile(vertices, indices);
        }
    };

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;
    virtual void buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) const override;

    virtual VboMesh* newMesh() const override {
        return new Mesh(m_vertexLayout, m_drawMode);
    };

    
public:
    
    PolygonStyle(GLenum _drawMode = GL_TRIANGLES);
    PolygonStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~PolygonStyle() {
    }
};
