#include "vboMesh.h"
#include "shaderProgram.h"
#include "renderState.h"

namespace Tangram {

int VboMesh::s_validGeneration = 0;

VboMesh::VboMesh() {
    m_drawMode = GL_TRIANGLES;
    m_hint = GL_STATIC_DRAW;
    m_keepMemoryData = false;
    m_glVertexBuffer = 0;
    m_glIndexBuffer = 0;
    m_nVertices = 0;
    m_nIndices = 0;
    m_dirtyOffset = 0;
    m_dirtySize = 0;

    m_dirty = false;
    m_isUploaded = false;
    m_isCompiled = false;

    m_generation = -1;
}

VboMesh::VboMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode, GLenum _hint, bool _keepMemoryData) : VboMesh() {
    m_vertexLayout = _vertexLayout;
    m_hint = _hint;
    m_keepMemoryData = _keepMemoryData;

    setDrawMode(_drawMode);
}

VboMesh::~VboMesh() {
    if (m_glVertexBuffer) {
        glDeleteBuffers(1, &m_glVertexBuffer);
    }
    if (m_glIndexBuffer) {
        glDeleteBuffers(1, &m_glIndexBuffer);
    }

    delete[] m_glVertexData;
    delete[] m_glIndexData;
}

void VboMesh::setVertexLayout(std::shared_ptr<VertexLayout> _vertexLayout) {
    m_vertexLayout = _vertexLayout;
}

void VboMesh::setDrawMode(GLenum _drawMode) {
    switch (_drawMode) {
        case GL_POINTS:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
        case GL_LINES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
        case GL_TRIANGLES:
            m_drawMode = _drawMode;
            break;
        default:
            logMsg("WARNING: Invalid draw mode for mesh! Defaulting to GL_TRIANGLES\n");
            m_drawMode = GL_TRIANGLES;
    }
}

void VboMesh::subDataUpload() {
    if (!m_dirty) {
        return false;
    }

    if (m_hint == GL_STATIC_DRAW) {
        logMsg("WARNING: wrong usage hint provided to the Vbo\n");
        assert(false);
    }

    RenderState::vertexBuffer(m_glVertexBuffer);

    long vertexBytes = m_nVertices * m_vertexLayout->getStride();

    // when all vertices are modified, it's better to update the entire mesh
    if (vertexBytes - m_dirtySize < m_vertexLayout->getStride()) {

        // invalidate the data store on the driver
        glBufferData(GL_ARRAY_BUFFER, vertexBytes, NULL, m_hint);

        // if this buffer is still used by gpu on current frame this call will not wait
        // for the frame to finish using the vbo but "directly" send command to upload the data
        glBufferData(GL_ARRAY_BUFFER, vertexBytes, m_glVertexData, m_hint);
    } else {
        // perform simple sub data upload for part of the buffer
        glBufferSubData(GL_ARRAY_BUFFER, m_dirtyOffset, m_dirtySize, m_glVertexData + m_dirtyOffset);
    }

    m_dirtyOffset = 0;
    m_dirtySize = 0;
    m_dirty = false;
}

void VboMesh::upload() {

    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) {
        glGenBuffers(1, &m_glVertexBuffer);
    }

    // Buffer vertex data
    int vertexBytes = m_nVertices * m_vertexLayout->getStride();

    RenderState::vertexBuffer(m_glVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, m_glVertexData, m_hint);

    // Clear vertex data that is not supposed to be updated.
    if (m_hint == GL_STATIC_DRAW && !m_keepMemoryData) {
        delete[] m_glVertexData;
        m_glVertexData = nullptr;
    }

    if (m_glIndexData) {

        if (m_glIndexBuffer == 0) {
            glGenBuffers(1, &m_glIndexBuffer);
        }

        // Buffer element index data
        RenderState::indexBuffer(m_glIndexBuffer);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_nIndices * sizeof(GLushort), m_glIndexData, m_hint);

        if (!m_keepMemoryData) {
            delete[] m_glIndexData;
            m_glIndexData = nullptr;
        }
    }

    m_generation = s_validGeneration;

    m_isUploaded = true;
}

void VboMesh::draw(ShaderProgram& _shader) {

    checkValidity();

    if (!m_isCompiled) { return; }
    if (m_nVertices == 0) { return; }

    // Ensure that geometry is buffered into GPU
    if (!m_isUploaded) {
        upload();
    } else if (m_dirty) {
        subDataUpload();
    }

    // Bind buffers for drawing
    RenderState::vertexBuffer(m_glVertexBuffer);

    if (m_nIndices > 0) {
        RenderState::indexBuffer(m_glIndexBuffer);
    }

    // Enable shader program
    _shader.use();

    size_t indiceOffset = 0;
    size_t vertexOffset = 0;

    for (auto& o : m_vertexOffsets) {
        uint32_t nIndices = o.first;
        uint32_t nVertices = o.second;

        size_t byteOffset = vertexOffset * m_vertexLayout->getStride();

        // Enable vertex attribs via vertex layout object
        m_vertexLayout->enable(_shader, byteOffset);

        // Draw as elements or arrays
        if (nIndices > 0) {
            glDrawElements(m_drawMode, nIndices, GL_UNSIGNED_SHORT,
                           (void*)(indiceOffset * sizeof(GLushort)));
        } else if (nVertices > 0) {
            glDrawArrays(m_drawMode, 0, nVertices);
        }

        vertexOffset += nVertices;
        indiceOffset += nIndices;
    }
}

void VboMesh::checkValidity() {
    if (m_generation != s_validGeneration) {
        m_isUploaded = false;
        m_glVertexBuffer = 0;
        m_glIndexBuffer = 0;

        m_generation = s_validGeneration;
    }
}

size_t VboMesh::bufferSize() {
    return m_nVertices * m_vertexLayout->getStride() + m_nIndices * sizeof(GLushort);
}


void VboMesh::invalidateAllVBOs() {

    ++s_validGeneration;

}

}
