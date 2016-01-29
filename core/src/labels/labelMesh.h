#pragma once

#include "gl/typedMesh.h"
#include "labels/label.h"

#include <memory>
#include <vector>

namespace Tangram {

class ShaderProgram;

class LabelMesh : public TypedMesh<Label::Vertex> {
public:
    LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode);

    virtual ~LabelMesh();

    void addLabel(std::unique_ptr<Label> _label);

    const std::vector<std::unique_ptr<Label>>& getLabels() const {
        return m_labels;
    }
    void compile();

    virtual void draw(ShaderProgram& _shader) override;

    void reset();

    void addVertices(std::vector<Label::Vertex>&& _vertices,
                     std::vector<uint16_t>&& _indices);

protected:
    void loadQuadIndices();

    std::vector<std::vector<Label::Vertex>> m_vertices;
    std::vector<std::vector<uint16_t>> m_indices;

    std::vector<std::unique_ptr<Label>> m_labels;
};

}
