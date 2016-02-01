#pragma once

#include "gl/vboMesh.h"
#include "labels/label.h"

#include <memory>
#include <vector>

namespace Tangram {

class ShaderProgram;

class LabelSet {
public:
    const std::vector<std::unique_ptr<Label>>& getLabels() const {
        return m_labels;
    }

    virtual ~LabelSet();

    void setLabels(std::vector<std::unique_ptr<Label>>& _labels);

    void reset();

protected:
    std::vector<std::unique_ptr<Label>> m_labels;
};

class LabelMesh : public VboMesh<Label::Vertex> {
public:
    LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode);

    virtual ~LabelMesh() override;

    virtual void draw(ShaderProgram& _shader) override;

    void compile(std::vector<Label::Vertex>& _vertices);

    static void loadQuadIndices();

};

}
