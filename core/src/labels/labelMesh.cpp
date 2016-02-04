#include "labels/labelMesh.h"
#include "labels/label.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include <atomic>

namespace Tangram {

static GLuint s_quadIndexBuffer = 0;
static int s_quadGeneration = -1;
static std::atomic<int> s_meshCounter(0);

const size_t maxVertices = 16384;

LabelMesh::LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
    : VboMesh<Label::Vertex>(_vertexLayout, _drawMode, GL_DYNAMIC_DRAW)
{
    s_meshCounter++;
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
    indices.reserve(maxVertices / 4 * 6);

    for (size_t i = 0; i < maxVertices; i += 4) {
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

void LabelMesh::compile(std::vector<Label::Vertex>& _vertices) {

    constexpr size_t maxVertices = 16384;

    // Compile vertex buffer directly instead of making a temporary copy
    m_nVertices = _vertices.size();

    int stride = m_vertexLayout->getStride();
    m_glVertexData = new GLbyte[stride * m_nVertices];
    std::memcpy(m_glVertexData,
                reinterpret_cast<const GLbyte*>(_vertices.data()),
                m_nVertices * stride);

    for (size_t offset = 0; offset < m_nVertices; offset += maxVertices) {
        size_t nVertices = maxVertices;
        if (offset + maxVertices > m_nVertices) {
            nVertices = m_nVertices - offset;
        }
        m_vertexOffsets.emplace_back(nVertices / 4 * 6, nVertices);
    }
    m_isCompiled = true;
}

void LabelMesh::draw(ShaderProgram& _shader) {
    bool valid = checkValidity();

    if (!m_isCompiled) { return; }
    if (m_nVertices == 0) { return; }

    // Ensure that geometry is buffered into GPU
    if (!m_isUploaded) {
        upload();
    } else if (m_dirty) {
        subDataUpload();
    }

    if (!valid) {
        loadQuadIndices();
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
