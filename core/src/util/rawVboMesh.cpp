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
    // shift indices by previous mesh vertices offset
    int indiceOffset = 0;
    int sumVertices = 0;
    int vPos = 0, iPos = 0;

    bool useIndices = m_nIndices > 0;

    int stride = m_vertexLayout->getStride();
    GLbyte* vBuffer = new GLbyte[stride * m_nVertices];
    GLushort* iBuffer = nullptr;

    if (useIndices) iBuffer = new GLushort[m_nIndices];

    for (size_t i = 0; i < m_vertices.size(); i++) {
        auto verts = m_vertices[i];
        int nBytes = verts.size();
        size_t nVertices = verts.size() / stride;

        std::memcpy(vBuffer + vPos, verts.data(), nBytes);
        vPos += nBytes;

        if (useIndices) {
            if (indiceOffset + nVertices > MAX_INDEX_VALUE) {
                logMsg("NOTICE: >>>>>> BIG MESH %d <<<<<<\n",
                       indiceOffset + nVertices);
                m_vertexOffsets.emplace_back(iPos, sumVertices);
                indiceOffset = 0;
            }
            auto ids = m_indices[i];
            int nElem = ids.size();
            for (int j = 0; j < nElem; j++) {
                iBuffer[iPos++] = ids[j] + indiceOffset;
            }
            ids.clear();
            indiceOffset += verts.size();
        }
        sumVertices += nVertices;
        verts.clear();
    }

    m_vertexOffsets.emplace_back(iPos, sumVertices);

    return { iBuffer, vBuffer };
}
