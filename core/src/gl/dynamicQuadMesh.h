#pragma once

#include "gl/mesh.h"
#include "gl/glError.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"
#include "gl/texture.h"
#include "gl/hardware.h"
#include "gl/vao.h"

#include <memory>
#include <vector>
#include <atomic>

namespace Tangram {

template<class T>
class DynamicQuadMesh : public StyledMesh, protected MeshBase {

public:

    DynamicQuadMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
        : MeshBase(_vertexLayout, _drawMode, GL_DYNAMIC_DRAW) {
    }

    bool draw(RenderState& rs, ShaderProgram& _shader, bool _useVao = true) override;

    bool drawRange(RenderState& rs, ShaderProgram& shader, size_t vertexPos, size_t vertexCount);

    size_t bufferSize() const override {
        return MeshBase::bufferSize();
    }

    void clear() {
        // Clear vertices for next frame
        m_nVertices = 0;
        m_isUploaded = false;
        m_vertices.clear();
        //m_batches.clear();
    }

    size_t numberOfVertices() const { return m_vertices.size(); }

    void upload(RenderState& rs) override;

    bool isReady() { return m_isUploaded; }

    // Reserves space for one quad and returns pointer
    // into m_vertices to write into 4 vertices.
    T* pushQuad() {
        m_nVertices += 4;
        m_vertices.insert(m_vertices.end(), 4, {});
        return &m_vertices[m_nVertices - 4];
    }

private:

    std::vector<T> m_vertices;
    Vao m_vaos;
};

template<class T>
void DynamicQuadMesh<T>::upload(RenderState& rs) {

    if (m_nVertices == 0 || m_isUploaded) { return; }

    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) {
        GL::genBuffers(1, &m_glVertexBuffer);
    }

    MeshBase::subDataUpload(rs, reinterpret_cast<GLbyte*>(m_vertices.data()));

    m_isUploaded = true;
}

template<class T>
bool DynamicQuadMesh<T>::draw(RenderState& rs, ShaderProgram& shader, bool useVao) {

    if (m_nVertices == 0) { return false; }

    // Enable shader program
    if (!shader.use(rs)) { return false; }

#ifdef DYNAMIC_MESH_VAOS
    useVao &= Hardware::supportsVAOs;

    if (useVao) {
        // Capture vao state for a default vertex offset of 0/0
        if (!m_vaos.isInitialized()) {
            VertexOffsets vertexOffsets;
            vertexOffsets.emplace_back(0, 0);
            m_vaos.initialize(rs, shader, vertexOffsets, *m_vertexLayout,
                              m_glVertexBuffer, rs.getQuadIndexBuffer());
        }
    }
#endif

    const size_t verticesIndexed = RenderState::MAX_QUAD_VERTICES;
    size_t vertexPos = 0;
    const size_t vertexBatchEnd = m_nVertices;

    // Draw vertices in batches until the end of the mesh.
    while (vertexPos < m_nVertices) {

        assert(vertexPos <= vertexBatchEnd);

        // Determine the largest batch of vertices we can draw at once,
        // limited by the max index value.
        size_t verticesInBatch = std::min(vertexBatchEnd - vertexPos, verticesIndexed);

        // Set up and draw the batch.
#ifdef DYNAMIC_MESH_VAOS
        if (useVao && vertexPos == 0) {
            // Use vao only for first batch of offsets, other vertices can use a
            // different stride so just reuse the vertex layout with a different
            // byte offset instead
            m_vaos.bind(0);
        } else
#endif
        {
            rs.vertexBuffer(m_glVertexBuffer);
            rs.indexBuffer(rs.getQuadIndexBuffer());

            size_t byteOffset = vertexPos * m_vertexLayout->getStride();
            m_vertexLayout->enable(rs, shader, byteOffset);
        }

        size_t elementsInBatch = verticesInBatch * 6 / 4;
        GL::drawElements(m_drawMode, elementsInBatch, GL_UNSIGNED_SHORT, 0);

#ifdef DYNAMIC_MESH_VAOS
        if (useVao && vertexPos == 0) {
            m_vaos.unbind();
        }
#endif

        // Update counters.
        vertexPos += verticesInBatch;
    }

    return true;
}

template<class T>
bool DynamicQuadMesh<T>::drawRange(RenderState& rs, ShaderProgram& shader,
                                   size_t vertexPos, size_t vertexCount) {

    if (m_nVertices == 0) { return false; }

    // Enable shader program
    if (!shader.use(rs)) { return false; }

    const size_t verticesIndexed = RenderState::MAX_QUAD_VERTICES;
    const size_t vertexBatchEnd = vertexPos + vertexCount;

    // Set up and draw the batch.
    rs.vertexBuffer(m_glVertexBuffer);
    rs.indexBuffer(rs.getQuadIndexBuffer());

    // Draw vertices in batches until the end of the mesh.
    while (vertexPos < vertexBatchEnd) {

        // Determine the largest batch of vertices we can draw at once,
        // limited by the max index value.
        size_t verticesInBatch = std::min(vertexBatchEnd - vertexPos, verticesIndexed);

        size_t byteOffset = vertexPos * m_vertexLayout->getStride();
        m_vertexLayout->enable(rs, shader, byteOffset);

        size_t elementsInBatch = verticesInBatch * 6 / 4;
        GL::drawElements(m_drawMode, elementsInBatch, GL_UNSIGNED_SHORT, 0);

        // Update counters.
        vertexPos += verticesInBatch;
    }

    return true;
}

}
