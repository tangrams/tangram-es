#pragma once

#include "gl.h"
#include "platform.h"
#include "vertexLayout.h"

#include <cstring> // for memcpy
#include <vector>
#include <memory>

#define MAX_INDEX_VALUE 65535 // Maximum value of GLushort

namespace Tangram {
/*
 * VboMesh - Drawable collection of geometry contained in a vertex buffer and (optionally) an index buffer
 */
class VboMesh {

public:

    /*
     * Creates a VboMesh for vertex data arranged in the structure described by _vertexLayout to be drawn
     * using the OpenGL primitive type _drawMode
     */
    VboMesh(std::shared_ptr<VertexLayout> _vertexlayout, GLenum _drawMode = GL_TRIANGLES, GLenum _hint = GL_STATIC_DRAW, bool _keepMemoryData = false);
    VboMesh();

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
    virtual ~VboMesh();

    int numVertices() const {
        return m_nVertices;
    }

    int numIndices() const {
        return m_nIndices;
    }

    virtual void compileVertexBuffer() = 0;

    /*
     * Copies all added vertices and indices into OpenGL buffer objects; After geometry is uploaded,
     * no more vertices or indices can be added
     */
    void upload();

    /*
     * Sub data upload of the mesh, returns true if this results in a buffer binding
     */
    void subDataUpload();

    /*
     * Renders the geometry in this mesh using the ShaderProgram _shader; if geometry has not already
     * been uploaded it will be uploaded at this point
     */
    virtual void draw(ShaderProgram& _shader);

    size_t bufferSize();

    static void addManagedVBO(VboMesh* _vbo);

    static void removeManagedVBO(VboMesh* _vbo);

    static void invalidateAllVBOs();

    GLsizei getDirtySize() const { return m_dirtySize; }
    GLintptr getDirtyOffset() const { return m_dirtyOffset; }

protected:

    static int s_validGeneration; // Incremented when the GL context is invalidated
    int m_generation; // Generation in which this mesh's GL handles were created

    // Used in draw for legth and offsets: sumIndices, sumVertices
    // needs to be set by compileVertexBuffers()
    std::vector<std::pair<uint32_t, uint32_t>> m_vertexOffsets;

    std::shared_ptr<VertexLayout> m_vertexLayout;

    size_t m_nVertices;
    GLuint m_glVertexBuffer;
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

    void checkValidity();

    template <typename T>
    void compile(std::vector<std::vector<T>>& _vertices,
                 std::vector<std::vector<uint16_t>>& _indices) {

        std::vector<std::vector<T>> vertices;
        std::vector<std::vector<uint16_t>> indices;

        // take over contents
        std::swap(_vertices, vertices);
        std::swap(_indices, indices);

        int vertexOffset = 0, indexOffset = 0;

        // Buffer positions: vertex byte and index short
        int vPos = 0, iPos = 0;

        int stride = m_vertexLayout->getStride();
        m_glVertexData = new GLbyte[stride * m_nVertices];

        bool useIndices = m_nIndices > 0;
        if (useIndices) {
            m_glIndexData = new GLushort[m_nIndices];
        }

        for (size_t i = 0; i < vertices.size(); i++) {
            auto& curVertices = vertices[i];
            size_t nVertices = curVertices.size();
            int nBytes = nVertices * stride;

            std::memcpy(m_glVertexData + vPos, (GLbyte*)curVertices.data(), nBytes);
            vPos += nBytes;

            if (useIndices) {
                if (vertexOffset + nVertices > MAX_INDEX_VALUE) {
                    logMsg("NOTICE: Big Mesh %d\n", vertexOffset + nVertices);

                    m_vertexOffsets.emplace_back(indexOffset, vertexOffset);
                    vertexOffset = 0;
                    indexOffset = 0;
                }

                for (int idx : indices[i]) {
                    m_glIndexData[iPos++] = idx + vertexOffset;
                }
                indexOffset += indices[i].size();
            }
            vertexOffset += nVertices;
        }

        m_vertexOffsets.emplace_back(indexOffset, vertexOffset);

        m_isCompiled = true;
    }
};

}
