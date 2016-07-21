#pragma once

#include "gl/mesh.h"
#include "gl/renderState.h"
#include "gl/shaderProgram.h"

#include <memory>
#include <vector>
#include <atomic>

namespace Tangram {

class QuadIndices {
public :
    static void load();

    static const size_t maxVertices = 16384;

private:
    static GLuint quadIndexBuffer;
    static int quadGeneration;
};


template<class T>
class DynamicQuadMesh : public StyledMesh, protected MeshBase {

public:

    DynamicQuadMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
        : MeshBase(_vertexLayout, _drawMode, GL_DYNAMIC_DRAW) {
    }

    bool draw(ShaderProgram& _shader) override;

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

    void upload() override;

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
void DynamicQuadMesh<T>::upload() {

    if (m_nVertices == 0 || m_isUploaded) { return; }

    MeshBase::checkValidity();

    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) {
        GL_CHECK(glGenBuffers(1, &m_glVertexBuffer));
    }

    MeshBase::subDataUpload(reinterpret_cast<GLbyte*>(m_vertices.data()));

    m_isUploaded = true;
}

template<class T>
bool DynamicQuadMesh<T>::draw(ShaderProgram& _shader) {

    if (m_nVertices == 0) { return false; }

    // Bind buffers for drawing
    RenderState::vertexBuffer(m_glVertexBuffer);
    QuadIndices::load();

    // Enable shader program
    if (!_shader.use()) {
        return false;
    }

    size_t vertexOffset = 0;

    for (size_t offset = 0; offset < m_nVertices; offset += QuadIndices::maxVertices) {
        size_t nVertices = QuadIndices::maxVertices;

        if (offset + QuadIndices::maxVertices > m_nVertices) {
            nVertices = m_nVertices - offset;
        }
        size_t byteOffset = vertexOffset * m_vertexLayout->getStride();

        m_vertexLayout->enable(_shader, byteOffset);

        GL_CHECK(glDrawElements(m_drawMode, nVertices * 6 / 4, GL_UNSIGNED_SHORT, 0));

        vertexOffset += nVertices;
    }

    return true;
}

}
