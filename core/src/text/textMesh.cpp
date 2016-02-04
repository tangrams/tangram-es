#include "textMesh.h"

namespace Tangram {

void TextMesh::pushQuad(GlyphQuad& _quad, Label::Vertex::State& _state) {
    m_vertices.resize(m_nVertices + 4);

    for (int i = 0; i < 4; i++) {
        Label::Vertex& v = m_vertices[m_nVertices+i];
        v.pos = _quad.quad[i].pos;
        v.uv = _quad.quad[i].uv;
        v.color = _quad.color;
        v.stroke = _quad.stroke;
        v.state = _state;
    }
    m_nVertices += 4;
}

void TextMesh::myUpload() {

    if (m_nVertices == 0) { return; }

    m_vertexOffsets.clear();
    for (size_t offset = 0; offset < m_nVertices; offset += maxLabelMeshVertices) {
        size_t nVertices = maxLabelMeshVertices;
        if (offset + maxLabelMeshVertices > m_nVertices) {
            nVertices = m_nVertices - offset;
        }
        m_vertexOffsets.emplace_back(nVertices / 4 * 6, nVertices);
    }

    if (!checkValidity()) {
        loadQuadIndices();
        bufferCapacity = 0;
    }

    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) { glGenBuffers(1, &m_glVertexBuffer); }

    // Buffer vertex data
    int vertexBytes = m_nVertices * m_vertexLayout->getStride();

    RenderState::vertexBuffer(m_glVertexBuffer);

    if (vertexBytes > bufferCapacity) {
        bufferCapacity = vertexBytes;

        glBufferData(GL_ARRAY_BUFFER, vertexBytes,
                     reinterpret_cast<GLbyte*>(m_vertices.data()),
                     m_hint);
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexBytes,
                        reinterpret_cast<GLbyte*>(m_vertices.data()));
    }
    m_isCompiled = true;
    m_isUploaded = true;
    m_dirty = false;
}

bool TextMesh::ready() {
    return m_isCompiled;
}

void TextMesh::clear() {
    m_nVertices = 0;
    m_vertices.clear();
    m_isCompiled = false;
}

}
