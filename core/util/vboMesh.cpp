#include "vboMesh.h"

VboMesh::VboMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode) : m_vertexLayout(_vertexLayout) {

    m_glVertexBuffer = 0;
    m_glIndexBuffer = 0;
    m_nVertices = 0;
    m_nIndices = 0;

    m_isUploaded = false;

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
            logMsg("%s\n","Invalid draw mode for mesh! Defaulting to GL_TRIANGLES");
            m_drawMode = GL_TRIANGLES;
    }

}

VboMesh::~VboMesh() {

    glDeleteBuffers(1, &m_glVertexBuffer);
    glDeleteBuffers(1, &m_glIndexBuffer);

}

void VboMesh::addVertex(GLbyte* _vertex) {

    addVertices(_vertex, 1);

}

void VboMesh::addVertices(GLbyte* _vertices, int _nVertices) {

    if (m_isUploaded) {
        logMsg("%s\n", "VboMesh cannot add vertices after upload!");
        return;
    }

    int vertexBytes = m_vertexLayout->getStride() * _nVertices;
    m_vertexData.insert(m_vertexData.cend(), _vertices, _vertices + vertexBytes);
    m_nVertices += _nVertices;

}

void VboMesh::addIndex(GLushort _index) {

    addIndices(&_index, 1);

}

void VboMesh::addIndices(GLushort* _indices, int _nIndices) {

    if (m_isUploaded) {
        logMsg("%s\n", "VboMesh cannot add indices after upload!");
        return;
    }

    m_indices.insert(m_indices.cend(), _indices, _indices + _nIndices);
    m_nIndices += _nIndices;

}

void VboMesh::upload() {

    if (m_nVertices > 0) {
        // Generate vertex buffer, if needed
        if (m_glVertexBuffer == 0) {
            glGenBuffers(1, &m_glVertexBuffer);
        }

        // Buffer vertex data
        glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, m_vertexData.size(), m_vertexData.data(), GL_STATIC_DRAW);
    }

    if (m_nIndices > 0) {
        // Generate index buffer, if needed
        if (m_glIndexBuffer == 0) {
            glGenBuffers(1, &m_glIndexBuffer);
        }

        // Buffer element index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GL_UNSIGNED_SHORT), m_indices.data(), GL_STATIC_DRAW);
    }

    // Release copies of geometry in CPU memory
    m_vertexData.clear();
    m_indices.clear();

    m_isUploaded = true;

}

void VboMesh::draw(std::shared_ptr<ShaderProgram> _shader) {

    // Ensure that geometry is buffered into GPU
    if (!m_isUploaded) {
        upload();
    }

    // Bind buffers for drawing
    if (m_nVertices > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
    }

    if (m_nIndices > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
    }

    // Enable shader program
    _shader->use();

    // Enable vertex attribs via vertex layout object
    m_vertexLayout->enable(_shader);

    // Draw as elements or arrays
    if (m_nIndices > 0) {
        glDrawElements(m_drawMode, m_nIndices, GL_UNSIGNED_SHORT, 0);
    } else if (m_nVertices > 0) {
        glDrawArrays(m_drawMode, 0, m_nVertices);
    }

}
