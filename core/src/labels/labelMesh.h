#pragma once

#include "gl/vboMesh.h"
#include "labels/label.h"

#include <memory>
#include <vector>

namespace Tangram {

class ShaderProgram;


class LabelMesh : public VboMesh<Label::Vertex> {

public:

    LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode);

    virtual ~LabelMesh() override;

    void draw(ShaderProgram& _shader, bool _clear = true) override;

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
