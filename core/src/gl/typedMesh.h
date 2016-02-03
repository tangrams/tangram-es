#pragma once

#include "vboMesh.h"

#include <cstring> // for memcpy
#include <cassert>

namespace Tangram {

template<class T>
struct MeshData {

    MeshData() {}
    MeshData(std::vector<uint16_t>&& _indices, std::vector<T>&& _vertices)
        : indices(std::move(_indices)),
          vertices(std::move(_vertices)) {
        offsets.emplace_back(indices.size(), vertices.size());
    }

    std::vector<uint16_t> indices;
    std::vector<T> vertices;
    std::vector<std::pair<uint32_t, uint32_t>> offsets;

    void clear() {
        offsets.clear();
        indices.clear();
        vertices.clear();
    }
};

template<class T>
class TypedMesh : public VboMesh {

public:

    TypedMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode,
              GLenum _hint = GL_STATIC_DRAW, bool _keepMemoryData = false)
        : VboMesh(_vertexLayout, _drawMode, _hint, _keepMemoryData) {};

    /*
     * Update _nVerts vertices in the mesh with the new T value _newVertexValue
     * starting after _byteOffset in the mesh vertex data memory
     */
    void updateVertices(Range _vertexRange, const T& _newVertexValue);

    /*
     * Update _nVerts vertices in the mesh with the new attribute A
     * _newAttributeValue starting after _byteOffset in the mesh vertex data
     * memory
     */
    template<class A>
    void updateAttribute(Range _vertexRange,
                         const A& _newAttributeValue,
                         size_t _attribOffset = 0);

    void compile(const std::vector<MeshData<T>>& _meshes);

    void compile(const MeshData<T>& _mesh);

protected:

    void setDirty(GLintptr _byteOffset, GLsizei _byteSize);
};


template<class T>
void TypedMesh<T>::compile(const std::vector<MeshData<T>>& _meshes) {

    m_nVertices = 0;
    m_nIndices = 0;

    for (auto& m : _meshes) {
        m_nVertices += m.vertices.size();
        m_nIndices += m.indices.size();
    }

    int stride = m_vertexLayout->getStride();
    m_glVertexData = new GLbyte[m_nVertices * stride];

    size_t offset = 0;
    for (auto& m : _meshes) {
        size_t nBytes = m.vertices.size() * stride;
        std::memcpy(m_glVertexData + offset,
                    (const GLbyte*)m.vertices.data(),
                    nBytes);

        offset += nBytes;
    }

    assert(offset == m_nVertices * stride);

    if (m_nIndices > 0) {
        m_glIndexData = new GLushort[m_nIndices];

        size_t offset = 0;
        for (auto& m : _meshes) {
            offset = compileIndices(m.offsets, m.indices, offset);
        }
        assert(offset == m_nIndices);
    }

    m_isCompiled = true;
}

template<class T>
void TypedMesh<T>::compile(const MeshData<T>& _mesh) {

    m_nVertices = _mesh.vertices.size();
    m_nIndices = _mesh.indices.size();

    int stride = m_vertexLayout->getStride();
    m_glVertexData = new GLbyte[m_nVertices * stride];

    std::memcpy(m_glVertexData,
                (const GLbyte*)_mesh.vertices.data(),
                m_nVertices * stride);

    if (m_nIndices > 0) {
        m_glIndexData = new GLushort[m_nIndices];
        compileIndices(_mesh.offsets, _mesh.indices, 0);
    }

    m_isCompiled = true;
}

template<class T>
template<class A>
void TypedMesh<T>::updateAttribute(Range _vertexRange,
                                   const A& _newAttributeValue,
                                   size_t _attribOffset) {
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
    if (size_t(_vertexRange.start + _vertexRange.length) > m_nVertices) {
        //LOGW("Invalid range");
        return;
    }
    if (_attribOffset >= tSize) {
        //LOGW("Invalid attribute offset");
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

    if (_vertexRange.start + _vertexRange.length > int(m_nVertices)) {
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
