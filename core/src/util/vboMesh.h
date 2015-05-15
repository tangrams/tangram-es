#pragma once

#include <vector>
#include <memory>

#include "gl.h"
#include "vertexLayout.h"
#include <cstring>

#define MAX_INDEX_VALUE 65535

/*
 * VboMesh - Drawable collection of geometry contained in a vertex buffer and (optionally) an index buffer
 */

class VboMesh {

public:

    /*
     * Creates a VboMesh for vertex data arranged in the structure described by _vertexLayout to be drawn
     * using the OpenGL primitive type _drawMode
     */
    VboMesh(std::shared_ptr<VertexLayout> _vertexlayout, GLenum _drawMode = GL_TRIANGLES);
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
     * Renders the geometry in this mesh using the ShaderProgram _shader; if geometry has not already
     * been uploaded it will be uploaded at this point
     */
    void draw(const std::shared_ptr<ShaderProgram> _shader);
    
    static void addManagedVBO(VboMesh* _vbo);
    
    static void removeManagedVBO(VboMesh* _vbo);
    
    static void invalidateAllVBOs();

protected:

    static int s_validGeneration; // Incremented when the GL context is invalidated
    int m_generation; // Generation in which this mesh's GL handles were created

    // Used in draw for legth and offsets: sumIndices, sumVertices
    // needs to be set by compileVertexBuffers()
    std::vector<std::pair<uint32_t, uint32_t>> m_vertexOffsets;

    std::shared_ptr<VertexLayout> m_vertexLayout;

    int m_nVertices;
    GLuint m_glVertexBuffer;
    // Compiled vertices for upload
    std::vector<GLbyte> m_glVertexData;

    int m_nIndices;
    GLuint m_glIndexBuffer;
    // Compiled  indices for upload
    std::vector<GLushort> m_glIndexData;

    GLenum m_drawMode;

    bool m_isUploaded;
    bool m_isCompiled;
    
    void checkValidity();

    template <typename T>
    void compile(std::vector<std::vector<T>>& _vertices,
                 std::vector<std::vector<int>>& _indices) {

        std::vector<std::vector<T>> vertices;
        std::vector<std::vector<int>> indices;

        // take over contents
        std::swap(_vertices, vertices);
        std::swap(_indices, indices);

        int vertexOffset = 0, indexOffset = 0;

        // Buffer positions: vertex byte and index short
        int vPos = 0, iPos = 0;

        int stride = m_vertexLayout->getStride();
        m_glVertexData.resize(stride * m_nVertices);
        GLbyte* vBuffer = m_glVertexData.data();

        GLushort* iBuffer = nullptr;
        bool useIndices = m_nIndices > 0;
        if (useIndices) {
            m_glIndexData.resize(m_nIndices);
            iBuffer = m_glIndexData.data();
        }

        for (size_t i = 0; i < vertices.size(); i++) {
            auto curVertices = vertices[i];
            size_t nVertices = curVertices.size();
            int nBytes = nVertices * stride;

            std::memcpy(vBuffer + vPos, (GLbyte*)curVertices.data(), nBytes);
            vPos += nBytes;

            if (useIndices) {
                if (vertexOffset + nVertices > MAX_INDEX_VALUE) {
                    logMsg("NOTICE: Big Mesh %d\n", vertexOffset + nVertices);

                    m_vertexOffsets.emplace_back(indexOffset, vertexOffset);
                    vertexOffset = 0;
                    indexOffset = 0;
                }

                for (int idx : indices[i]) {
                    iBuffer[iPos++] = idx + vertexOffset;
                }
                indexOffset += indices[i].size();
            }
            vertexOffset += nVertices;
        }

        m_vertexOffsets.emplace_back(indexOffset, vertexOffset);

        m_isCompiled = true;
    }
};
