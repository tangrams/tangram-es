#pragma once

#include "gl/mesh.h"
#include "gl/error.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"

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

    bool draw(RenderState& rs, ShaderProgram& _shader) override;

    size_t bufferSize() const override {
        return MeshBase::bufferSize();
    }

    void clear() {
        // Clear vertices for next frame
        m_nVertices = 0;
        m_isUploaded = false;
        m_vertices.clear();
    }

    size_t numberOfVertices() const { return m_vertices.size(); }

    void upload(RenderState& rs) override;

    bool isReady() { return m_isUploaded; }

    // Reserves space for one quad and returns pointer
    // into m_vertices to write into 4 vertices.
    T* pushQuad() {
        m_nVertices += 4;
        m_vertices.resize(m_nVertices);
        return &m_vertices[m_nVertices - 4];
    }

private:

    std::vector<T> m_vertices;
};

template<class T>
void DynamicQuadMesh<T>::upload(RenderState& rs) {

    if (m_nVertices == 0 || m_isUploaded) { return; }

    MeshBase::checkValidity(rs);

    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) {
        GL_CHECK(glGenBuffers(1, &m_glVertexBuffer));
    }

    MeshBase::subDataUpload(rs, reinterpret_cast<GLbyte*>(m_vertices.data()));

    m_isUploaded = true;
}

template<class T>
bool DynamicQuadMesh<T>::draw(RenderState& rs, ShaderProgram& _shader) {

    if (m_nVertices == 0) { return false; }

    // Bind buffers for drawing
    rs.vertexBuffer(m_glVertexBuffer);
    rs.indexBuffer(rs.getQuadIndexBuffer());

    // Enable shader program
    if (!_shader.use(rs)) {
        return false;
    }

    size_t vertexOffset = 0;
    size_t maxVertices = RenderState::MAX_QUAD_VERTICES;

    for (size_t offset = 0; offset < m_nVertices; offset += maxVertices) {
        size_t nVertices = maxVertices;

        if (offset + maxVertices > m_nVertices) {
            nVertices = m_nVertices - offset;
        }
        size_t byteOffset = vertexOffset * m_vertexLayout->getStride();

        m_vertexLayout->enable(rs, _shader, byteOffset);

        GL_CHECK(glDrawElements(m_drawMode, nVertices * 6 / 4, GL_UNSIGNED_SHORT, 0));

        vertexOffset += nVertices;
    }

    return true;
}

}
