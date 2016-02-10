#include "labels/labelMesh.h"
#include "labels/label.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "gl/hardware.h"
#include <atomic>

namespace Tangram {

static GLuint s_quadIndexBuffer = 0;
static int s_quadGeneration = -1;
static std::atomic<int> s_meshCounter(0);

LabelMesh::LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
    : VboMesh<Label::Vertex>(_vertexLayout, _drawMode, GL_DYNAMIC_DRAW)
{
    s_meshCounter++;
    m_isCompiled = true;
}

void LabelMesh::pushQuad(GlyphQuad& _quad, Label::Vertex::State& _state) {
    m_vertices.resize(m_nVertices + 4);

    for (int i = 0; i < 4; i++) {
        Label::Vertex& v = m_vertices[m_nVertices+i];
        v.pos = _quad.quad[i].pos;
        v.uv = _quad.quad[i].uv;
        v.color = _quad.color;
        v.stroke = _quad.stroke; // union with extrude
        v.state = _state;
    }

    m_nVertices += 4;
}

LabelMesh::~LabelMesh() {
    s_meshCounter--;

    if (s_quadIndexBuffer != 0 && (!RenderState::isCurrentGeneration(s_quadGeneration) || s_meshCounter <= 0)) {
        if (RenderState::indexBuffer.compare(s_quadIndexBuffer)) {
            RenderState::indexBuffer.init(0, false);
        }
        glDeleteBuffers(1, &s_quadIndexBuffer);
        s_quadIndexBuffer = 0;
        s_quadGeneration = -1;
    }
}

void LabelMesh::loadQuadIndices() {
    if (RenderState::isCurrentGeneration(s_quadGeneration)) {
        return;
    }

    s_quadGeneration = RenderState::generation();

    std::vector<GLushort> indices;
    indices.reserve(maxLabelMeshVertices / 4 * 6);

    for (size_t i = 0; i < maxLabelMeshVertices; i += 4) {
        indices.push_back(i + 2);
        indices.push_back(i + 0);
        indices.push_back(i + 1);
        indices.push_back(i + 1);
        indices.push_back(i + 3);
        indices.push_back(i + 2);
    }

    glGenBuffers(1, &s_quadIndexBuffer);
    RenderState::indexBuffer(s_quadIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort),
                 reinterpret_cast<GLbyte*>(indices.data()), GL_STATIC_DRAW);
}

void LabelMesh::myUpload() {
    if (m_nVertices == 0) { return; }

    m_vertexOffsets.clear();
    for (size_t offset = 0; offset < m_nVertices; offset += maxLabelMeshVertices) {
        size_t nVertices = maxLabelMeshVertices;
        if (offset + maxLabelMeshVertices > m_nVertices) {
            nVertices = m_nVertices - offset;
        }
        m_vertexOffsets.emplace_back(nVertices / 4 * 6, nVertices);
    }

    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) {
        glGenBuffers(1, &m_glVertexBuffer);
    }

    // Buffer vertex data
    int vertexBytes = m_nVertices * m_vertexLayout->getStride();

    RenderState::vertexBuffer(m_glVertexBuffer);

    GLbyte* bufferData = reinterpret_cast<GLbyte*>(m_vertices.data());

    // invalidate/orphane the data store on the driver
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, NULL, m_hint);

    if (Hardware::supportsMapBuffer) {
        GLvoid* dataStore = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

        // write memory client side
        std::memcpy(dataStore, bufferData, vertexBytes);

        glUnmapBuffer(GL_ARRAY_BUFFER);
    } else {
        glBufferData(GL_ARRAY_BUFFER, vertexBytes, bufferData, m_hint);
    }
}

void LabelMesh::clear() {
    // Clear vertices for next frame
    m_nVertices = 0;
    m_vertices.clear();
}

void LabelMesh::draw(ShaderProgram& _shader) {
    bool valid = checkValidity();

    if (m_nVertices == 0) { return; }

    if (!valid) {
        loadQuadIndices();
    }

    if (!m_isUploaded) {
        myUpload();
    }

    // Bind buffers for drawing
    RenderState::vertexBuffer(m_glVertexBuffer);
    RenderState::indexBuffer(s_quadIndexBuffer);

    // Enable shader program
    _shader.use();

    size_t vertexOffset = 0;

    for (size_t i = 0; i < m_vertexOffsets.size(); ++i) {
        auto& o = m_vertexOffsets[i];
        uint32_t nIndices = o.first;
        uint32_t nVertices = o.second;

        size_t byteOffset = vertexOffset * m_vertexLayout->getStride();

        m_vertexLayout->enable(_shader, byteOffset);

        glDrawElements(m_drawMode, nIndices, GL_UNSIGNED_SHORT, 0);

        vertexOffset += nVertices;
    }
}

}
