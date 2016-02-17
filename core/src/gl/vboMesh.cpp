#include "vboMesh.h"
#include "shaderProgram.h"
#include "renderState.h"
#include "hardware.h"
#include "platform.h"

namespace Tangram {


VboMeshBase::VboMeshBase() {
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

VboMeshBase::VboMeshBase(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode,
                 GLenum _hint, bool _keepMemoryData) : VboMeshBase() {
    m_vertexLayout = _vertexLayout;
    m_hint = _hint;
    m_keepMemoryData = _keepMemoryData;

    setDrawMode(_drawMode);
}

VboMeshBase::~VboMeshBase() {
    // Deleting a index/array buffer being used ends up setting up the current vertex/index buffer to 0
    // after the driver finishes using it, force the render state to be 0 for vertex/index buffer

    if (m_glVertexBuffer) {
        if (RenderState::vertexBuffer.compare(m_glVertexBuffer)) {
            RenderState::vertexBuffer.init(0, false);
        }
        glDeleteBuffers(1, &m_glVertexBuffer);
    }
    if (m_glIndexBuffer) {
        if (RenderState::indexBuffer.compare(m_glIndexBuffer)) {
            RenderState::indexBuffer.init(0, false);
        }
        glDeleteBuffers(1, &m_glIndexBuffer);
    }

    delete[] m_glVertexData;
    delete[] m_glIndexData;
}

void VboMeshBase::setVertexLayout(std::shared_ptr<VertexLayout> _vertexLayout) {
    m_vertexLayout = _vertexLayout;
}

void VboMeshBase::setDrawMode(GLenum _drawMode) {
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
            LOGW("Invalid draw mode for mesh! Defaulting to GL_TRIANGLES");
            m_drawMode = GL_TRIANGLES;
    }
}

void VboMeshBase::subDataUpload(GLbyte* _data) {
    if (!m_dirty) {
        return;
    }

    if (m_hint == GL_STATIC_DRAW) {
        LOGW("Wrong usage hint provided to the Vbo");
        assert(false);
    }

    GLbyte* data = _data ? _data : m_glVertexData;

    RenderState::vertexBuffer(m_glVertexBuffer);

    long vertexBytes = m_nVertices * m_vertexLayout->getStride();

    // invalidate/orphane the data store on the driver
    glBufferData(GL_ARRAY_BUFFER, vertexBytes, NULL, m_hint);

    if (Hardware::supportsMapBuffer) {
        GLvoid* dataStore = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

        // write memory client side
        std::memcpy(dataStore, data, vertexBytes);

        glUnmapBuffer(GL_ARRAY_BUFFER);
    } else {

        // if this buffer is still used by gpu on current frame this call will not wait
        // for the frame to finish using the vbo but "directly" send command to upload the data
        glBufferData(GL_ARRAY_BUFFER, vertexBytes, data, m_hint);
    }

    m_dirty = false;
}

void VboMeshBase::upload() {

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

    m_generation = RenderState::generation();

    m_isUploaded = true;
}

void VboMeshBase::draw(ShaderProgram& _shader, bool _clear) {

    checkValidity();

    if (!m_isCompiled) { return; }
    if (m_nVertices == 0) { return; }

    // Enable shader program
    if (!_shader.use()) {
        return;
    }

    // Ensure that geometry is buffered into GPU
    if (!m_isUploaded) {
        upload();
    } else if (m_dirty) {
        subDataUpload();
    }

    if (Hardware::supportsVAOs) {
        if (!m_vaos) {
            m_vaos = std::make_unique<Vao>();

            // Capture vao state
            m_vaos->init(_shader, m_vertexOffsets, *m_vertexLayout, m_glVertexBuffer, m_glIndexBuffer);
        }
    } else {
        // Bind buffers for drawing
        RenderState::vertexBuffer(m_glVertexBuffer);

        if (m_nIndices > 0) {
            RenderState::indexBuffer(m_glIndexBuffer);
        }
    }

    size_t indiceOffset = 0;
    size_t vertexOffset = 0;

    for (size_t i = 0; i < m_vertexOffsets.size(); ++i) {
        auto& o = m_vertexOffsets[i];
        uint32_t nIndices = o.first;
        uint32_t nVertices = o.second;

        if (!Hardware::supportsVAOs) {
            // Enable vertex attribs via vertex layout object
            size_t byteOffset = vertexOffset * m_vertexLayout->getStride();
            m_vertexLayout->enable(_shader, byteOffset);
        } else {
            // Bind the corresponding vao relative to the current offset
            m_vaos->bind(i);
        }

        // Draw as elements or arrays
        if (nIndices > 0) {
            glDrawElements(m_drawMode, nIndices, GL_UNSIGNED_SHORT, (void*)(indiceOffset * sizeof(GLushort)));
        } else if (nVertices > 0) {
            glDrawArrays(m_drawMode, 0, nVertices);
        }

        vertexOffset += nVertices;
        indiceOffset += nIndices;
    }

    if (Hardware::supportsVAOs) {
        m_vaos->unbind();
    }
}

bool VboMeshBase::checkValidity() {
    if (!RenderState::isCurrentGeneration(m_generation)) {
        m_isUploaded = false;
        m_glVertexBuffer = 0;
        m_glIndexBuffer = 0;
        m_vaos.reset();

        m_generation = RenderState::generation();

        return false;
    }

    return true;
}

size_t VboMeshBase::bufferSize() {
    return m_nVertices * m_vertexLayout->getStride() + m_nIndices * sizeof(GLushort);
}

// Add indices by collecting them into batches to draw as much as
// possible in one draw call.  The indices must be shifted by the
// number of vertices that are present in the current batch.
size_t VboMeshBase::compileIndices(const std::vector<std::pair<uint32_t, uint32_t>>& _offsets,
                                   const std::vector<uint16_t>& _indices, size_t _offset) {

    m_vertexOffsets.emplace_back(0, 0);

    GLushort* dst = m_glIndexData + _offset;
    size_t curVertices = 0;
    size_t src = 0;

    for (auto& p : _offsets) {
        size_t nIndices = p.first;
        size_t nVertices = p.second;

        if (curVertices + nVertices > MAX_INDEX_VALUE) {
            m_vertexOffsets.emplace_back(0, 0);
            curVertices = 0;
        }
        for (size_t i = 0; i < nIndices; i++, dst++) {
            *dst = _indices[src++] + curVertices;
        }

        auto& offset = m_vertexOffsets.back();
        offset.first += nIndices;
        offset.second += nVertices;

        curVertices += nVertices;
    }

    return _offset + src;
}

void VboMeshBase::setDirty(GLintptr _byteOffset, GLsizei _byteSize) {

    if (!m_dirty) {
        m_dirty = true;

        m_dirtySize = _byteSize;
        m_dirtyOffset = _byteOffset;

    } else {
        size_t end = std::max(m_dirtyOffset + m_dirtySize, _byteOffset + _byteSize);

        m_dirtyOffset = std::min(m_dirtyOffset, _byteOffset);
        m_dirtySize = end - m_dirtyOffset;
    }
}

}
