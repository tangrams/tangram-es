#include "vboMesh.h"
#include "platform.h"

#define MAX_INDEX_VALUE 65535 // Maximum value of GLushort

int VboMesh::s_validGeneration = 0;

VboMesh::VboMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
    : m_vertexLayout(_vertexLayout) {

    m_glVertexBuffer = 0;
    m_glIndexBuffer = 0;
    m_nVertices = 0;
    m_nIndices = 0;

    m_isUploaded = false;
    m_isCompiled = false;

    setDrawMode(_drawMode);
}

VboMesh::VboMesh() {
    m_glVertexBuffer = 0;
    m_glIndexBuffer = 0;
    m_nVertices = 0;
    m_nIndices = 0;

    m_isUploaded = false;
    m_isCompiled = false;
}

VboMesh::~VboMesh() {
    if (m_glVertexBuffer) glDeleteBuffers(1, &m_glVertexBuffer);
    if (m_glIndexBuffer) glDeleteBuffers(1, &m_glIndexBuffer);
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

void VboMesh::upload() {
    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) {
        glGenBuffers(1, &m_glVertexBuffer);
    }

    // TODO check if compiled?
    // Buffer vertex data
    int vertexBytes = m_glVertexData.size();

    glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, m_glVertexData.data(), GL_STATIC_DRAW);

    if (!m_glIndexData.empty()) {
        
        if (m_glIndexBuffer == 0) {
            glGenBuffers(1, &m_glIndexBuffer);
        }

        // Buffer element index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_glIndexData.size() * sizeof(GLushort),
                     m_glIndexData.data(), GL_STATIC_DRAW);
    }

    // m_glVertexData.resize(0);
    // m_glIndexData.resize(0);

    // TODO: For now, we retain copies of the vertex and index data in CPU memory to allow VBOs
    // to easily rebuild themselves after GL context loss. For optimizing memory usage (and for
    // other reasons) we'll want to change this in the future. This probably means going back to
    // data sources and styles to rebuild the vertex data.
    
    m_generation = s_validGeneration;

    m_isUploaded = true;

}

void VboMesh::draw(const std::shared_ptr<ShaderProgram> _shader) {

    checkValidity();

    if (!m_isCompiled) return;

    if (m_nVertices == 0) return;

    // Ensure that geometry is buffered into GPU
    if (!m_isUploaded) {
        upload();
    }

    // Bind buffers for drawing
    glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);

    if (m_nIndices > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
    }

    // Enable shader program
    _shader->use();

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

void VboMesh::invalidateAllVBOs() {
    
    ++s_validGeneration;
    
}
