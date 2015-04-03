#include "rawVboMesh.h"

void RawVboMesh::addVertex(GLbyte* _vertex) {
    addVertices(_vertex, 1);
}

void RawVboMesh::addVertices(GLbyte* _vertices, int _nVertices) {

    if (m_isUploaded) {
        logMsg("ERROR: RawVboMesh cannot add vertices after upload!\n");
        return;
    }

    // Only add up to 65535 vertices, any more will overflow our 16-bit indices
    int indexSpace = MAX_INDEX_VALUE - m_nVertices;

    // FIXME only relevant when using indices
    if (_nVertices > MAX_INDEX_VALUE) {
        logMsg("WARNING: Cannot add > %d vertices in one call, truncating mesh\n", MAX_INDEX_VALUE);
        _nVertices = indexSpace;
    }

    int vertexBytes = m_vertexLayout->getStride() * _nVertices;

    m_vertices.emplace_back(_vertices, _vertices + vertexBytes);
    m_nVertices += _nVertices;
}

void RawVboMesh::addIndex(int* _index) {
    addIndices(_index, 1);
}

void RawVboMesh::addIndices(int* _indices, int _nIndices) {

    if (m_isUploaded) {
        logMsg("ERROR: RawVboMesh cannot add indices after upload!\n");
        return;
    }

    m_indices.emplace_back(_indices, _indices + _nIndices);
    m_nIndices += _nIndices;
}

VboMesh::ByteBuffers RawVboMesh::compileVertexBuffer() {
    int stride = m_vertexLayout->getStride();
    return compile(m_vertices, m_indices, stride);
}
