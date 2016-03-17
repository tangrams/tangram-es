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
    static void ref();
    static void unref();

    static const size_t maxVertices = 16384;

private:
    static GLuint quadIndexBuffer;
    static int quadGeneration;

    static std::atomic<int> meshCounter;
};


template<class T>
class DynamicQuadMesh : public Mesh<T> {
    // Make base class members visible in template
    // What's all that 'using'?
    // http://stackoverflow.com/questions/4643074/why-do-i-have-to
    // -access-template-base-class-members-through-the-this-pointer
    using Mesh<T>::m_isUploaded;
    using Mesh<T>::m_drawMode;
    using Mesh<T>::m_nVertices;
    using Mesh<T>::m_glVertexBuffer;
    using Mesh<T>::m_vertexOffsets;
    using Mesh<T>::m_vertexLayout;

public:

    DynamicQuadMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
        : Mesh<T>(_vertexLayout, _drawMode, GL_DYNAMIC_DRAW) {

        QuadIndices::ref();
    }

    ~DynamicQuadMesh() override {
        QuadIndices::unref();
    }

    void draw(ShaderProgram& _shader) override;

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

    Mesh<T>::checkValidity();

    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) {
        glGenBuffers(1, &m_glVertexBuffer);
    }

    Mesh<T>::subDataUpload(reinterpret_cast<GLbyte*>(m_vertices.data()));

    m_isUploaded = true;
}

template<class T>
void DynamicQuadMesh<T>::draw(ShaderProgram& _shader) {

    if (m_nVertices == 0) { return; }

    // Bind buffers for drawing
    RenderState::vertexBuffer(m_glVertexBuffer);
    QuadIndices::load();

    // Enable shader program
    _shader.use();

    size_t vertexOffset = 0;

    for (size_t offset = 0; offset < m_nVertices; offset += QuadIndices::maxVertices) {
        size_t nVertices = QuadIndices::maxVertices;

        if (offset + QuadIndices::maxVertices > m_nVertices) {
            nVertices = m_nVertices - offset;
        }
        size_t byteOffset = vertexOffset * m_vertexLayout->getStride();

        m_vertexLayout->enable(_shader, byteOffset);

        glDrawElements(m_drawMode, nVertices * 6 / 4, GL_UNSIGNED_SHORT, 0);

        vertexOffset += nVertices;
    }
}

}
