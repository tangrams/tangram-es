#pragma once

#include "gl.h"
#include "vertexLayout.h"
#include "vao.h"
#include "util/types.h"
#include "platform.h"
#include "style/style.h"

#include <string>
#include <vector>
#include <memory>
#include <cstring> // for memcpy
#include <cassert>

#define MAX_INDEX_VALUE 65535 // Maximum value of GLushort

namespace Tangram {

/*
 * VboMesh - Drawable collection of geometry contained in a vertex buffer and (optionally) an index buffer
 */
struct VboMeshBase {
public:
    /*
     * Creates a VboMesh for vertex data arranged in the structure described by
     * _vertexLayout to be drawn using the OpenGL primitive type _drawMode
     */
    VboMeshBase(std::shared_ptr<VertexLayout> _vertexlayout, GLenum _drawMode = GL_TRIANGLES,
                GLenum _hint = GL_STATIC_DRAW, bool _keepMemoryData = false);

    VboMeshBase();

    /*
     * Set Vertex Layout for the vboMesh object
     */
    void setVertexLayout(std::shared_ptr<VertexLayout> _vertexLayout);

    /*
     * Set Draw mode for the vboMesh object
     */
    void setDrawMode(GLenum _drawMode = GL_TRIANGLES);

    /*
     * Destructs this VboMesh and releases all associated OpenGL resources
     */
    ~VboMeshBase();

    /*
     * Copies all added vertices and indices into OpenGL buffer objects; After
     * geometry is uploaded, no more vertices or indices can be added
     */
    void upload();

    /*
     * Sub data upload of the mesh, returns true if this results in a buffer binding
     */
    void subDataUpload();
    void resetDirty();

    /*
     * Renders the geometry in this mesh using the ShaderProgram _shader; if
     * geometry has not already been uploaded it will be uploaded at this point
     */
    void draw(ShaderProgram& _shader);

    size_t bufferSize();

protected:

    int m_generation; // Generation in which this mesh's GL handles were created

    // Used in draw for legth and offsets: sumIndices, sumVertices
    // needs to be set by compile()
    std::vector<std::pair<uint32_t, uint32_t>> m_vertexOffsets;

    std::shared_ptr<VertexLayout> m_vertexLayout;

    size_t m_nVertices;
    GLuint m_glVertexBuffer;

    std::unique_ptr<Vao> m_vaos;

    // Compiled vertices for upload
    GLbyte* m_glVertexData = nullptr;

    size_t m_nIndices;
    GLuint m_glIndexBuffer;
    // Compiled  indices for upload
    GLushort* m_glIndexData = nullptr;

    GLenum m_drawMode;
    GLenum m_hint;

    bool m_isUploaded;
    bool m_isCompiled;
    bool m_dirty;
    bool m_keepMemoryData;

    GLsizei m_dirtySize;
    GLintptr m_dirtyOffset;

    bool checkValidity();

    size_t compileIndices(const std::vector<std::pair<uint32_t, uint32_t>>& _offsets,
                          const std::vector<uint16_t>& _indices, size_t _offset);

    void setDirty(GLintptr _byteOffset, GLsizei _byteSize);
};

template<class T>
struct MeshData {

    MeshData() {}
    MeshData(std::vector<uint16_t>&& _indices, std::vector<T>&& _vertices)
        : indices(std::move(_indices)),
          vertices(std::move(_vertices)) {
        offsets.emplace_back(indices.size(), vertices.size());
    }

    std::vector<uint16_t> indices;
    std::vector<T> vertices;
    std::vector<std::pair<uint32_t, uint32_t>> offsets;

    void clear() {
        offsets.clear();
        indices.clear();
        vertices.clear();
    }
};

template<class T>
class VboMesh : public StyledMesh, protected VboMeshBase {
public:

    VboMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode,
              GLenum _hint = GL_STATIC_DRAW, bool _keepMemoryData = false)
        : VboMeshBase(_vertexLayout, _drawMode, _hint, _keepMemoryData) {};

    virtual ~VboMesh() {}

    virtual size_t bufferSize() {
        return VboMeshBase::bufferSize();
    }

    virtual void draw(ShaderProgram& _shader) {
        VboMeshBase::draw(_shader);
    }

    void compile(const std::vector<MeshData<T>>& _meshes);

    void compile(const MeshData<T>& _mesh);

    /*
     * Update _nVerts vertices in the mesh with the new T value _newVertexValue
     * starting after _byteOffset in the mesh vertex data memory
     */
    void updateVertices(Range _vertexRange, const T& _newVertexValue);

    /*
     * Update _nVerts vertices in the mesh with the new attribute A
     * _newAttributeValue starting after _byteOffset in the mesh vertex data
     * memory
     */
    template<class A>
    void updateAttribute(Range _vertexRange,
                         const A& _newAttributeValue,
                         GLbyte* _glBufferData = nullptr,
                         size_t _attribOffset = 0);

protected:

};


template<class T>
void VboMesh<T>::compile(const std::vector<MeshData<T>>& _meshes) {

    m_nVertices = 0;
    m_nIndices = 0;

    for (auto& m : _meshes) {
        m_nVertices += m.vertices.size();
        m_nIndices += m.indices.size();
    }

    int stride = m_vertexLayout->getStride();
    m_glVertexData = new GLbyte[m_nVertices * stride];

    size_t offset = 0;
    for (auto& m : _meshes) {
        size_t nBytes = m.vertices.size() * stride;
        std::memcpy(m_glVertexData + offset,
                    (const GLbyte*)m.vertices.data(),
                    nBytes);

        offset += nBytes;
    }

    assert(offset == m_nVertices * stride);

    if (m_nIndices > 0) {
        m_glIndexData = new GLushort[m_nIndices];

        size_t offset = 0;
        for (auto& m : _meshes) {
            offset = compileIndices(m.offsets, m.indices, offset);
        }
        assert(offset == m_nIndices);
    }

    m_isCompiled = true;
}

template<class T>
void VboMesh<T>::compile(const MeshData<T>& _mesh) {

    m_nVertices = _mesh.vertices.size();
    m_nIndices = _mesh.indices.size();

    int stride = m_vertexLayout->getStride();
    m_glVertexData = new GLbyte[m_nVertices * stride];

    std::memcpy(m_glVertexData,
                (const GLbyte*)_mesh.vertices.data(),
                m_nVertices * stride);

    if (m_nIndices > 0) {
        m_glIndexData = new GLushort[m_nIndices];
        compileIndices(_mesh.offsets, _mesh.indices, 0);
    }

    m_isCompiled = true;
}

template<class T>
template<class A>
void VboMesh<T>::updateAttribute(Range _vertexRange,
                                 const A& _newAttributeValue,
                                 GLbyte* _glBufferData,
                                 size_t _attribOffset) {

    GLbyte* data = _glBufferData ? _glBufferData : m_glVertexData;

    if (data == nullptr) {
        assert(false);
        return;
    }

    const size_t aSize = sizeof(A);
    const size_t tSize = sizeof(T);
    static_assert(aSize <= tSize, "Invalid attribute size");

    if (_vertexRange.start < 0 || _vertexRange.length < 1) {
        return;
    }
    if (size_t(_vertexRange.start + _vertexRange.length) > m_nVertices) {
        //LOGW("Invalid range");
        return;
    }
    if (_attribOffset >= tSize) {
        //LOGW("Invalid attribute offset");
        return;
    }

    size_t start = _vertexRange.start * tSize + _attribOffset;
    size_t end = start + _vertexRange.length * tSize;

    // update the vertices attributes
    for (size_t offset = start; offset < end; offset += tSize) {
        std::memcpy(data + offset, &_newAttributeValue, aSize);
    }

    // set all modified vertices dirty
    setDirty(start, (_vertexRange.length - 1) * tSize + aSize);
}

template<class T>
void VboMesh<T>::updateVertices(Range _vertexRange, const T& _newVertexValue) {
    if (m_glVertexData == nullptr) {
        assert(false);
        return;
    }

    size_t tSize = sizeof(T);

    if (_vertexRange.start + _vertexRange.length > int(m_nVertices)) {
        return;
    }


    size_t start = _vertexRange.start * tSize;
    size_t end = start + _vertexRange.length * tSize;

    // update the vertices
    for (size_t offset = start; offset < end; offset += tSize) {
        std::memcpy(m_glVertexData + offset, &_newVertexValue, tSize);
    }

    setDirty(start, end - start);
}

}
