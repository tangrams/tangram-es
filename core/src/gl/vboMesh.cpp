#include "vboMesh.h"
#include "shaderProgram.h"

namespace Tangram {

int VboMesh::s_validGeneration = 0;

VboMesh::VboMesh() {
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

VboMesh::VboMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode, GLenum _hint) : VboMesh() {
    m_vertexLayout = _vertexLayout;
    m_hint = _hint;

    setDrawMode(_drawMode);
}

VboMesh::~VboMesh() {
    if (m_glVertexBuffer) glDeleteBuffers(1, &m_glVertexBuffer);
    if (m_glIndexBuffer) glDeleteBuffers(1, &m_glIndexBuffer);

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

void VboMesh::update(GLintptr _offset, GLsizei _size, unsigned char* _data) {
    std::memcpy(m_glVertexData + _offset, _data, _size);

    m_dirtyOffset = _offset;
    m_dirtySize = _size;

    m_dirty = true;
}

bool VboMesh::subDataUpload() {
    if (!m_dirty) {
        return false;
    }

    if (m_hint == GL_STATIC_DRAW) {
        logMsg("WARNING: wrong usage hint provided to the Vbo\n");
        assert(false);
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);

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

    // Also bind indices when return 'already bound'.
    if (m_nIndices > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
    }

    return true;
}

bool VboMesh::upload() {
    if (m_isUploaded) {
        return false;
    }

    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) {
        glGenBuffers(1, &m_glVertexBuffer);
    }

    // Buffer vertex data
    int vertexBytes = m_nVertices * m_vertexLayout->getStride();

    glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, m_glVertexData, m_hint);

    // Clear vertex data that is not supposed to be updated.
    if (m_hint == GL_STATIC_DRAW) {
        delete[] m_glVertexData;
        m_glVertexData = nullptr;
    }

    if (m_glIndexData) {

        if (m_glIndexBuffer == 0) {
            glGenBuffers(1, &m_glIndexBuffer);
        }

        // Buffer element index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_nIndices * sizeof(GLushort), m_glIndexData, m_hint);

        delete[] m_glIndexData;
        m_glIndexData = nullptr;
    }

    m_generation = s_validGeneration;

    m_isUploaded = true;

    return true;

}

void VboMesh::draw(ShaderProgram& _shader) {

    checkValidity();

    if (!m_isCompiled) return;

    if (m_nVertices == 0) return;

    bool bound = false;

    // Ensure that geometry is buffered into GPU
    if (!m_isUploaded) {
        bound = upload();
    } else if (m_dirty) {
        bound = subDataUpload();
    }

    if (!bound) {
        // Bind buffers for drawing
        glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);

        if (m_nIndices > 0) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
        }
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
