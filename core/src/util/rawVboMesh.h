#include "vboMesh.h"

class RawVboMesh : public VboMesh {

 public:
  RawVboMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode = GL_TRIANGLES)
    : VboMesh(_vertexLayout, _drawMode){}

    /*
     * Adds a single vertex to the mesh; _vertex must be a pointer to the beginning of a vertex structured
     * according to the VertexLayout associated with this mesh
     */
    void addVertex(GLbyte* _vertex);

    /*
     * Adds _nVertices vertices to the mesh; _vertices must be a pointer to the beginning of a contiguous
     * block of _nVertices vertices structured according to the VertexLayout associated with this mesh
     */
    void addVertices(GLbyte* _vertices, int _nVertices);

     /*
     * Adds a single index to the mesh; indices are unsigned shorts
     */
    void addIndex(int* _index);

    /*
     * Adds _nIndices indices to the mesh; _indices must be a pointer to the beginning of a contiguous
     * block of _nIndices unsigned short indices
     */
    void addIndices(int* _indices, int _nIndices);

   protected:
    // Raw interleaved vertex data in the format specified by the vertex layout
    std::vector<std::vector<GLubyte>> m_vertices;

    std::vector<std::vector<int>> m_indices;

    virtual ByteBuffers compileVertexBuffer() override;
};
