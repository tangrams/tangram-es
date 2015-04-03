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
    auto vertices = new GLubyte[vertexBytes];
    std::memcpy(vertices, _vertices, vertexBytes);

    m_newVertices.emplace_back(vertices, vertexBytes);
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

    auto indices = new GLushort[_nIndices];
    for (int i = 0; i < _nIndices; i++) indices[i] = _indices[i];

    m_newIndices.emplace_back(indices, _nIndices);
    m_nIndices += _nIndices;
}

VboMesh::ByteBuffers
RawVboMesh::compileVertexBuffer() {
    int vertexBytes = m_vertexLayout->getStride() * m_nVertices;
    auto vertices = new GLubyte[vertexBytes];
    int offset = 0;
    for (auto p : m_newVertices) {
        std::memcpy(vertices + offset, p.first, p.second);
        offset += p.second;
        delete[] p.first;
    }
    m_newVertices.clear();

    auto indices = new GLushort[m_nIndices];
    offset = 0;
    for (auto p : m_newIndices) {
      std::memcpy(indices + offset, p.first, p.second * sizeof(GLushort));
      offset += p.second;
      delete[] p.first;
    }
    m_newIndices.clear();

    m_vertexOffsets.emplace_back(m_nIndices, m_nVertices);

    return { indices, vertices };
}

RawVboMesh::~RawVboMesh() {
    for (auto p : m_newVertices)
      delete[] p.first;

    for (auto p : m_newIndices)
      delete[] p.first;
}
