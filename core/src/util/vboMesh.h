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

  // Compiled vertices + indices ready for upload
  typedef std::pair<GLushort*, GLbyte*> ByteBuffers;

  virtual ByteBuffers compileVertexBuffer() = 0;

    static int s_validGeneration; // Incremented when the GL context is invalidated
    int m_generation;

    // Used in draw for legth and offsets: sumIndices, sumVertices
    // needs to be set by compileVertexBuffers()
    std::vector<std::pair<uint32_t, uint32_t>> m_vertexOffsets;

    std::shared_ptr<VertexLayout> m_vertexLayout;

    int m_nVertices;
    GLuint m_glVertexBuffer;

    int m_nIndices;
    GLuint m_glIndexBuffer;

    GLenum m_drawMode;

    bool m_isUploaded;
    
    void checkValidity();

    template <typename T>
    ByteBuffers compile(std::vector<std::vector<T>> vertices,
                        std::vector<std::vector<int>> indices,
                         int divider = 1) {

        int vertexOffset = 0;
        int indiceOffset = 0;

        // Buffer positions: vertex byte and index short
        int vPos = 0, iPos = 0;

        int stride = m_vertexLayout->getStride();
        GLbyte* vBuffer = new GLbyte[stride * m_nVertices];

        GLushort* iBuffer = nullptr;
        bool useIndices = m_nIndices > 0;
        if (useIndices)
          iBuffer = new GLushort[m_nIndices];

        for (size_t i = 0; i < vertices.size(); i++) {
            auto curVertices = vertices[i];
            size_t nVertices = curVertices.size() / divider;
            int nBytes = curVertices.size() * (stride / divider);

            std::memcpy(vBuffer + vPos, (GLbyte*)curVertices.data(), nBytes);
            vPos += nBytes;

            if (useIndices) {
                if (vertexOffset + nVertices > MAX_INDEX_VALUE) {
                    logMsg("NOTICE: Big Mesh %d\n",
                           vertexOffset + nVertices);

                    m_vertexOffsets.emplace_back(indiceOffset, vertexOffset);
                    vertexOffset = 0;
                    indiceOffset = 0;
                }

                for (int idx : indices[i])
                    iBuffer[iPos++] = idx + vertexOffset;

                indiceOffset += indices[i].size();

                indices[i].clear();
            }
            vertexOffset += nVertices;
            curVertices.clear();
        }

        m_vertexOffsets.emplace_back(indiceOffset, vertexOffset);

        return { iBuffer, vBuffer };
    }
};
