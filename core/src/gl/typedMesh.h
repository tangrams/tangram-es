#pragma once

#include "vboMesh.h"
#include "util/types.h"

#include <cstdlib> // std::abs
#include <cassert>

namespace Tangram {

template<class T>
class TypedMesh : public VboMesh {

public:

    TypedMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode, GLenum _hint = GL_STATIC_DRAW, bool _keepMemoryData = false)
        : VboMesh(_vertexLayout, _drawMode, _hint, _keepMemoryData) {};

    void addVertices(std::vector<T>&& _vertices,
                     std::vector<uint16_t>&& _indices) {

        m_nVertices += _vertices.size();
        m_nIndices += _indices.size();

        m_vertices.push_back(std::move(_vertices));
        m_indices.push_back(std::move(_indices));
    }

    virtual void compileVertexBuffer() override {
        compile(m_vertices, m_indices);
    }

    /*
     * Update _nVerts vertices in the mesh with the new T value _newVertexValue starting after
     * _byteOffset in the mesh vertex data memory
     */
    void updateVertices(Range _vertexRange, const T& _newVertexValue);

    /*
     * Update _nVerts vertices in the mesh with the new attribute A _newAttributeValue starting
     * after _byteOffset in the mesh vertex data memory
     */
    template<class A>
    void updateAttribute(Range _vertexRange, const A& _newAttributeValue, size_t _attribOffset = 0) {
        if (m_glVertexData == nullptr) {
            assert(false);
            return;
        }

        const size_t aSize = sizeof(A);
        const size_t tSize = sizeof(T);
        static_assert(aSize <= tSize, "Invalid attribute size");

        if (_vertexRange.start < 0 || _vertexRange.length < 1) {
            return;
        }
        if (_vertexRange.start + _vertexRange.length > m_nVertices) {
            LOGW("Invalid range");
            return;
        }
        if (_attribOffset >= tSize) {
            LOGW("Invalid attribute offset");
            return;
        }

        size_t start = _vertexRange.start * tSize + _attribOffset;
        size_t end = start + _vertexRange.length * tSize;

        // update the vertices attributes
        for (size_t offset = start; offset < end; offset += tSize) {
            std::memcpy(m_glVertexData + offset, &_newAttributeValue, aSize);
        }

        // set all modified vertices dirty
        setDirty(start, (_vertexRange.length - 1) * tSize + aSize);
    }

protected:

    void setDirty(GLintptr _byteOffset, GLsizei _byteSize);

    std::vector<std::vector<T>> m_vertices;
    std::vector<std::vector<uint16_t>> m_indices;

};

template<class T>
void TypedMesh<T>::setDirty(GLintptr _byteOffset, GLsizei _byteSize) {

    if (!m_dirty) {
        m_dirty = true;

        m_dirtySize = _byteSize;
        m_dirtyOffset = _byteOffset;

    } else {
        size_t end = std::max(m_dirtyOffset + m_dirtySize, _byteOffset + _byteSize);

        m_dirtyOffset = std::min(m_dirtyOffset, _byteOffset);
        m_dirtySize = end - m_dirtyOffset;
    }
}

template<class T>
void TypedMesh<T>::updateVertices(Range _vertexRange, const T& _newVertexValue) {
    if (m_glVertexData == nullptr) {
        assert(false);
        return;
    }

    size_t tSize = sizeof(T);

    if (_vertexRange.start +_vertexRange.length > m_nVertices) {
        return;
    }


    size_t start = _vertexRange.start * tSize;
    size_t end = start + _vertexRange.length * tSize;

    // update the vertices
    for (size_t offset = start; offset < end; offset += tSize) {
        std::memcpy(m_glVertexData + offset, &_newVertexValue, tSize);
    }

    setDirty(start, end - start);
}

}
