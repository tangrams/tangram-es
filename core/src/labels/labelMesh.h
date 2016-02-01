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

    virtual ~LabelMesh();

    const std::vector<std::unique_ptr<Label>>& getLabels() const {
        return m_labels;
    }

    virtual void draw(ShaderProgram& _shader) override;

    void compile(std::vector<std::unique_ptr<Label>>& _labels,
                 std::vector<Label::Vertex>& _vertices);

    void reset();

protected:
    void loadQuadIndices();

    std::vector<std::unique_ptr<Label>> m_labels;
};

}
