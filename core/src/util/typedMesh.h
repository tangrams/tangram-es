#pragma once

#include "vboMesh.h"

template<class T>
class TypedMesh : public VboMesh {

public:
    
    TypedMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
        : VboMesh(_vertexLayout, _drawMode){};

    void addVertices(std::vector<T>&& _vertices,
                     std::vector<int>&& _indices) {
        vertices.push_back(_vertices);
        indices.push_back(_indices);

        m_nVertices += _vertices.size();
        m_nIndices += _indices.size();
    }

    virtual void compileVertexBuffer() override {
        compile(vertices, indices);
    }

protected:
    
    std::vector<std::vector<T>> vertices;
    std::vector<std::vector<int>> indices;
    
};
