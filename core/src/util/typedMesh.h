#pragma once

#include "vboMesh.h"
#include <cstdlib> // std::abs

template<class T>
class TypedMesh : public VboMesh {

public:

    TypedMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode, GLenum _hint = GL_STATIC_DRAW)
        : VboMesh(_vertexLayout, _drawMode, _hint) {};

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

    /*
     * Update _nVerts vertices in the mesh with the new T value _newVertexValue starting after
     * _byteOffset in the mesh vertex data memory
     */
    void updateVertices(GLintptr _byteOffset, unsigned int _nVerts, const T& _newVertexValue);

    /*
     * Update _nVerts vertices in the mesh with the new attribute A _newAttributeValue starting
     * after _byteOffset in the mesh vertex data memory
     */
    template<class A>
    void updateAttribute(GLintptr _byteOffset, unsigned int _nVerts, const A& _newAttributeValue) {
        if (!m_isCompiled) {
            return;
        }

        size_t aSize = sizeof(A);
        size_t tSize = sizeof(T);

        // update the vertices attributes
        for (int i = 0; i < _nVerts; ++i) {
            std::memcpy(m_glVertexData + _byteOffset + i * tSize, &_newAttributeValue, aSize);
        }

        setDirty(_byteOffset + aSize, _nVerts * tSize + aSize);
    }

protected:

    void setDirty(GLintptr _byteOffset, GLsizei _byteSize);

    std::vector<std::vector<T>> vertices;
    std::vector<std::vector<int>> indices;

};

template<class T>
void TypedMesh<T>::setDirty(GLintptr _byteOffset, GLsizei _byteSize) {

    // not dirty at all, init the dirtiness of the buffer
    if (!m_dirty) {

        m_dirtySize = _byteSize;
        m_dirtyOffset = _byteOffset;
        m_dirty = true;

    } else {
        GLsizei dBytes = std::abs(_byteOffset - m_dirtyOffset); // distance in bytes
        GLintptr nOff = _byteOffset + _byteSize; // new offset
        GLintptr pOff = m_dirtySize + m_dirtyOffset; // previous offset

        if (_byteOffset < m_dirtyOffset) { // left part of the buffer

            // update before the old buffer offset
            m_dirtyOffset = _byteOffset;

            // merge sizes
            if (nOff > pOff) {
                m_dirtySize = _byteSize;
            } else {
                m_dirtySize += dBytes;
            }

            m_dirty = true;

        } else if (nOff > pOff) { // right part of the buffer

            // update starting after the old buffer offset
            m_dirtySize = dBytes + _byteSize;
            m_dirty = true;
        }
    }

}

template<class T>
void TypedMesh<T>::updateVertices(GLintptr _byteOffset, unsigned int _nVerts, const T& _newVertexValue) {
    if (!m_isCompiled) {
        return;
    }

    size_t tSize = sizeof(T);

    // update the vertices
    for (int i = 0; i < _nVerts; ++i) {
        std::memcpy(m_glVertexData + _byteOffset + i * tSize, &_newVertexValue, tSize);
    }

    setDirty(_byteOffset, _nVerts * tSize);
}
