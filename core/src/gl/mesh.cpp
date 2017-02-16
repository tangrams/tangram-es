#include "gl/mesh.h"
#include "gl/shaderProgram.h"
#include "gl/renderState.h"
#include "gl/hardware.h"
#include "gl/error.h"
#include "platform.h"
#include "log.h"

namespace Tangram {


MeshBase::MeshBase() {
    m_drawMode = GL_TRIANGLES;
    m_hint = GL_STATIC_DRAW;
    m_glVertexBuffer = 0;
    m_glIndexBuffer = 0;
    m_nVertices = 0;
    m_nIndices = 0;
    m_dirtyOffset = 0;
    m_dirtySize = 0;

    m_dirty = false;
    m_isUploaded = false;
    m_isCompiled = false;
}

MeshBase::MeshBase(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode, GLenum _hint)
    : MeshBase()
{
    m_vertexLayout = _vertexLayout;
    m_hint = _hint;

    setDrawMode(_drawMode);
}

MeshBase::~MeshBase() {

    auto vaos = m_vaos;
    auto glVertexBuffer = m_glVertexBuffer;
    auto glIndexBuffer = m_glIndexBuffer;

    m_disposer([=](RenderState& rs) mutable {
        // Deleting a index/array buffer being used ends up setting up the current vertex/index buffer to 0
        // after the driver finishes using it, force the render state to be 0 for vertex/index buffer
        if (glVertexBuffer) {
            rs.vertexBufferUnset(glVertexBuffer);
            GL::deleteBuffers(1, &glVertexBuffer);
        }
        if (glIndexBuffer) {
            rs.indexBufferUnset(glIndexBuffer);
            GL::deleteBuffers(1, &glIndexBuffer);
        }
        vaos.dispose();
    });


    if (m_glVertexData) {
        delete[] m_glVertexData;
    }

    if (m_glIndexData) {
        delete[] m_glIndexData;
    }

}

void MeshBase::setVertexLayout(std::shared_ptr<VertexLayout> _vertexLayout) {
    m_vertexLayout = _vertexLayout;
}

void MeshBase::setDrawMode(GLenum _drawMode) {
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

void MeshBase::subDataUpload(RenderState& rs, GLbyte* _data) {

    if (!m_dirty && _data == nullptr) { return; }

    if (m_hint == GL_STATIC_DRAW) {
        LOGW("Wrong usage hint provided to the Vbo");
        assert(false);
    }

    GLbyte* data = _data ? _data : m_glVertexData;

    rs.vertexBuffer(m_glVertexBuffer);

    long vertexBytes = m_nVertices * m_vertexLayout->getStride();

    // invalidate/orphane the data store on the driver
    GL::bufferData(GL_ARRAY_BUFFER, vertexBytes, NULL, m_hint);

    if (Hardware::supportsMapBuffer) {
        GLvoid* dataStore = GL::mapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

        // write memory client side
        std::memcpy(dataStore, data, vertexBytes);

        GL::unmapBuffer(GL_ARRAY_BUFFER);
    } else {

        // if this buffer is still used by gpu on current frame this call will not wait
        // for the frame to finish using the vbo but "directly" send command to upload the data
        GL::bufferData(GL_ARRAY_BUFFER, vertexBytes, data, m_hint);
    }

    m_dirty = false;
}

void MeshBase::upload(RenderState& rs) {

    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) {
        GL::genBuffers(1, &m_glVertexBuffer);
    }

    // Buffer vertex data
    int vertexBytes = m_nVertices * m_vertexLayout->getStride();

    rs.vertexBuffer(m_glVertexBuffer);
    GL::bufferData(GL_ARRAY_BUFFER, vertexBytes, m_glVertexData, m_hint);

    delete[] m_glVertexData;
    m_glVertexData = nullptr;

    if (m_glIndexData) {

        if (m_glIndexBuffer == 0) {
            GL::genBuffers(1, &m_glIndexBuffer);
        }

        // Buffer element index data
        rs.indexBuffer(m_glIndexBuffer);

        GL::bufferData(GL_ELEMENT_ARRAY_BUFFER, m_nIndices * sizeof(GLushort), m_glIndexData, m_hint);

        delete[] m_glIndexData;
        m_glIndexData = nullptr;
    }

    m_disposer = Disposer(rs);

    m_isUploaded = true;
}

bool MeshBase::draw(RenderState& rs, ShaderProgram& _shader, bool _useVao) {
    bool useVao = _useVao && Hardware::supportsVAOs;

    if (!m_isCompiled) { return false; }
    if (m_nVertices == 0) { return false; }

    // Enable shader program
    if (!_shader.use(rs)) {
        return false;
    }

    // Ensure that geometry is buffered into GPU
    if (!m_isUploaded) {
        upload(rs);
    } else if (m_dirty) {
        subDataUpload(rs);
    }

    if (useVao) {
        if (!m_vaos.isInitialized()) {
            // Capture vao state
            m_vaos.initialize(rs, _shader, m_vertexOffsets, *m_vertexLayout, m_glVertexBuffer, m_glIndexBuffer);
        }
    } else {
        // Bind buffers for drawing
        rs.vertexBuffer(m_glVertexBuffer);

        if (m_nIndices > 0) {
            rs.indexBuffer(m_glIndexBuffer);
        }
    }

    size_t indiceOffset = 0;
    size_t vertexOffset = 0;

    for (size_t i = 0; i < m_vertexOffsets.size(); ++i) {
        auto& o = m_vertexOffsets[i];
        uint32_t nIndices = o.first;
        uint32_t nVertices = o.second;

        if (!useVao) {
            // Enable vertex attribs via vertex layout object
            size_t byteOffset = vertexOffset * m_vertexLayout->getStride();
            m_vertexLayout->enable(rs,  _shader, byteOffset);
        } else {
            // Bind the corresponding vao relative to the current offset
            m_vaos.bind(i);
        }

        // Draw as elements or arrays
        if (nIndices > 0) {
            GL::drawElements(m_drawMode, nIndices, GL_UNSIGNED_SHORT,
                             (void*)(indiceOffset * sizeof(GLushort)));
        } else if (nVertices > 0) {
            GL::drawArrays(m_drawMode, 0, nVertices);
        }

        vertexOffset += nVertices;
        indiceOffset += nIndices;
    }

    if (useVao) {
        m_vaos.unbind();
    }

    return true;
}

size_t MeshBase::bufferSize() const {
    return m_nVertices * m_vertexLayout->getStride() + m_nIndices * sizeof(GLushort);
}

// Add indices by collecting them into batches to draw as much as
// possible in one draw call.  The indices must be shifted by the
// number of vertices that are present in the current batch.
size_t MeshBase::compileIndices(const std::vector<std::pair<uint32_t, uint32_t>>& _offsets,
                                const std::vector<uint16_t>& _indices, size_t _offset) {


    GLushort* dst = m_glIndexData + _offset;
    size_t curVertices = 0;
    size_t src = 0;

    if (m_vertexOffsets.empty()) {
        m_vertexOffsets.emplace_back(0, 0);
    } else {
        curVertices = m_vertexOffsets.back().second;
    }

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

void MeshBase::setDirty(GLintptr _byteOffset, GLsizei _byteSize) {

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
