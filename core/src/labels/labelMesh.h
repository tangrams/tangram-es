#pragma once

#include "gl/mesh.h"
#include "labels/label.h"

#include <memory>
#include <vector>

namespace Tangram {

class ShaderProgram;


class LabelMesh : public Mesh<Label::Vertex> {

public:

    LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode);

    virtual ~LabelMesh() override;

    void draw(ShaderProgram& _shader) override;

    void compile(std::vector<Label::Vertex>& _vertices);

    void clear();

    size_t numberOfVertices() const { return m_vertices.size(); }

    void myUpload();

    bool isReady() { return m_isUploaded; }

    // Reserves space for one quad and returns pointer
    // into m_vertices to write into 4 vertices.
    Label::Vertex* pushQuad();

private:

    static void loadQuadIndices();

    std::vector<Label::Vertex> m_vertices;
};

}
