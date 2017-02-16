#pragma once

#include "gl.h"
#include "gl/disposer.h"
#include "gl/vertexLayout.h"
#include "gl/vao.h"
#include "style/style.h"
#include "util/types.h"
#include "platform.h"

#include <string>
#include <vector>
#include <memory>
#include <cstring> // for memcpy
#include <cassert>

#define MAX_INDEX_VALUE 65535 // Maximum value of GLushort

namespace Tangram {

/*
 * Mesh - Drawable collection of geometry contained in a vertex buffer and
 * (optionally) an index buffer
 */
struct MeshBase {
public:
    /*
     * Creates a Mesh for vertex data arranged in the structure described by
     * _vertexLayout to be drawn using the OpenGL primitive type _drawMode
     */
    MeshBase(std::shared_ptr<VertexLayout> _vertexlayout, GLenum _drawMode = GL_TRIANGLES,
             GLenum _hint = GL_STATIC_DRAW);

    MeshBase();

    MeshBase(const MeshBase&) = delete;
    MeshBase(MeshBase&&) = delete;
    MeshBase& operator=(const MeshBase&) = delete;
    MeshBase& operator=(MeshBase&&) = delete;

    virtual ~MeshBase();

    /*
     * Set Vertex Layout for the mesh object
     */
    void setVertexLayout(std::shared_ptr<VertexLayout> _vertexLayout);

    /*
     * Set Draw mode for the mesh object
     */
    void setDrawMode(GLenum _drawMode = GL_TRIANGLES);

    /*
     * Releases all OpenGL resources for this mesh
     */
    void dispose(RenderState& rs);

    /*
     * Copies all added vertices and indices into OpenGL buffer objects; After
     * geometry is uploaded, no more vertices or indices can be added
     */
    virtual void upload(RenderState& rs);

    /*
     * Sub data upload of the mesh, returns true if this results in a buffer binding
     */
    void subDataUpload(RenderState& rs, GLbyte* _data = nullptr);

    /*
     * Renders the geometry in this mesh using the ShaderProgram _shader; if
     * geometry has not already been uploaded it will be uploaded at this point
     */
    bool draw(RenderState& rs, ShaderProgram& _shader, bool _useVao = true);

    size_t bufferSize() const;

protected:

    // Used in draw for legth and offsets: sumIndices, sumVertices
    // needs to be set by compile()
    std::vector<std::pair<uint32_t, uint32_t>> m_vertexOffsets;

    std::shared_ptr<VertexLayout> m_vertexLayout;

    size_t m_nVertices;
    GLuint m_glVertexBuffer;

    Vao m_vaos;

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

    GLsizei m_dirtySize;
    GLintptr m_dirtyOffset;

    Disposer m_disposer;

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
class Mesh : public StyledMesh, protected MeshBase {
public:

    Mesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode,
         GLenum _hint = GL_STATIC_DRAW)
        : MeshBase(_vertexLayout, _drawMode, _hint) {};

    virtual ~Mesh() {}

    size_t bufferSize() const override {
        return MeshBase::bufferSize();
    }

    bool draw(RenderState& rs, ShaderProgram& shader, bool useVao = true) override {
        return MeshBase::draw(rs, shader, useVao);
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
    void updateAttribute(Range _vertexRange, const A& _newAttributeValue,
                         size_t _attribOffset = 0);
};


template<class T>
void Mesh<T>::compile(const std::vector<MeshData<T>>& _meshes) {

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
void Mesh<T>::compile(const MeshData<T>& _mesh) {

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
void Mesh<T>::updateAttribute(Range _vertexRange, const A& _newAttributeValue,
                              size_t _attribOffset) {

    if (m_glVertexData == nullptr) {
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
        std::memcpy(m_glVertexData + offset, &_newAttributeValue, aSize);
    }

    // set all modified vertices dirty
    setDirty(start, (_vertexRange.length - 1) * tSize + aSize);
}

template<class T>
void Mesh<T>::updateVertices(Range _vertexRange, const T& _newVertexValue) {
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
