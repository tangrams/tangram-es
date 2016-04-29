#include "mesh.h"
#include "shaderProgram.h"
#include "renderState.h"
#include "hardware.h"
#include "platform.h"

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

    m_generation = -1;
}

MeshBase::MeshBase(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode, GLenum _hint)
    : MeshBase()
{
    m_vertexLayout = _vertexLayout;
    m_hint = _hint;

    setDrawMode(_drawMode);
}

MeshBase::~MeshBase() {
    // Deleting a index/array buffer being used ends up setting up the current vertex/index buffer to 0
    // after the driver finishes using it, force the render state to be 0 for vertex/index buffer

    if (m_glVertexBuffer) {
        if (RenderState::vertexBuffer.compare(m_glVertexBuffer)) {
            RenderState::vertexBuffer.init(0, false);
        }
        GL_CHECK(glDeleteBuffers(1, &m_glVertexBuffer));
    }
    if (m_glIndexBuffer) {
        if (RenderState::indexBuffer.compare(m_glIndexBuffer)) {
            RenderState::indexBuffer.init(0, false);
        }
        GL_CHECK(glDeleteBuffers(1, &m_glIndexBuffer));
    }

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

void MeshBase::subDataUpload(GLbyte* _data) {

    if (!m_dirty && _data == nullptr) { return; }

    if (m_hint == GL_STATIC_DRAW) {
        LOGW("Wrong usage hint provided to the Vbo");
        assert(false);
    }

    GLbyte* data = _data ? _data : m_glVertexData;

    RenderState::vertexBuffer(m_glVertexBuffer);

    long vertexBytes = m_nVertices * m_vertexLayout->getStride();

    // invalidate/orphane the data store on the driver
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, vertexBytes, NULL, m_hint));

    if (Hardware::supportsMapBuffer) {
        GLvoid* dataStore = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        GL_CHECK(void(0));

        // write memory client side
        std::memcpy(dataStore, data, vertexBytes);

        GL_CHECK(glUnmapBuffer(GL_ARRAY_BUFFER));
    } else {

        // if this buffer is still used by gpu on current frame this call will not wait
        // for the frame to finish using the vbo but "directly" send command to upload the data
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, vertexBytes, data, m_hint));
    }

    m_dirty = false;
}

void MeshBase::upload() {

    // Generate vertex buffer, if needed
    if (m_glVertexBuffer == 0) {
        GL_CHECK(glGenBuffers(1, &m_glVertexBuffer));
    }

    // Buffer vertex data
    int vertexBytes = m_nVertices * m_vertexLayout->getStride();

    RenderState::vertexBuffer(m_glVertexBuffer);
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, vertexBytes, m_glVertexData, m_hint));

    delete[] m_glVertexData;
    m_glVertexData = nullptr;

    if (m_glIndexData) {

        if (m_glIndexBuffer == 0) {
            GL_CHECK(glGenBuffers(1, &m_glIndexBuffer));
        }

        // Buffer element index data
        RenderState::indexBuffer(m_glIndexBuffer);

        GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_nIndices * sizeof(GLushort), m_glIndexData, m_hint));

        delete[] m_glIndexData;
        m_glIndexData = nullptr;
    }

    m_generation = RenderState::generation();

    m_isUploaded = true;
}

bool MeshBase::draw(ShaderProgram& _shader) {

    checkValidity();

    if (!m_isCompiled) { return false; }
    if (m_nVertices == 0) { return false; }

    // Enable shader program
    if (!_shader.use()) {
        return false;
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
            GL_CHECK(glDrawElements(m_drawMode, nIndices, GL_UNSIGNED_SHORT,
                (void*)(indiceOffset * sizeof(GLushort))));
        } else if (nVertices > 0) {
            GL_CHECK(glDrawArrays(m_drawMode, 0, nVertices));
        }

        vertexOffset += nVertices;
        indiceOffset += nIndices;
    }

    if (Hardware::supportsVAOs) {
        m_vaos->unbind();
    }

    return true;
}

bool MeshBase::checkValidity() {
    if (!RenderState::isValidGeneration(m_generation)) {
        m_isUploaded = false;
        m_glVertexBuffer = 0;
        m_glIndexBuffer = 0;
        m_vaos.reset();

        m_generation = RenderState::generation();

        return false;
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
