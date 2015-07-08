#pragma once

#include "vboMesh.h"

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
    
    void updateVertices(GLintptr _byteOffset, unsigned int _nVerts, const T& _newVertexValue);
    
    template<class A>
    void updateAttribute(GLintptr _byteOffset, unsigned int _nVerts, const A& _newAttributeValue) {
        if (!m_isCompiled) {
            return;
        }
        
        size_t aSize = sizeof(A);
        size_t tSize = sizeof(T);
        
        // update the vertices
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
    // not dirty, init the dirtyness of the buffer
    if(!m_dirty) {
        m_dirtySize = _byteSize;
        m_dirtyOffset = _byteOffset;
        m_dirty = true;
    } else {
        // distance in bytes
        long dBytes = std::abs((long double)_byteOffset - m_dirtyOffset);
        long newEnd = _byteOffset + _byteSize;
        long oldEnd = m_dirtySize + m_dirtyOffset;
        
        if(_byteOffset < m_dirtyOffset) {
            // update from left part of the buffer
            m_dirtyOffset = _byteOffset;
            
            // merge sizes
            if(newEnd > oldEnd) {
                m_dirtySize = _byteSize;
            } else {
                m_dirtySize += dBytes;
            }
            m_dirty = true;
        } else if(newEnd > oldEnd) {
            // update from right part of the buffer
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
