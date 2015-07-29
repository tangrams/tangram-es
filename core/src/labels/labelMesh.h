#pragma once

#include "gl/typedMesh.h"
#include "labels/label.h"

#include <memory>
#include <vector>

namespace Tangram {

class LabelMesh : public TypedMesh<Label::Vertex> {
public:
    LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode);

    virtual ~LabelMesh();

    void addLabel(std::unique_ptr<Label> _label);

    const std::vector<std::unique_ptr<Label>>& getLabels() const {
        return m_labels;
    }

protected:
    std::vector<std::unique_ptr<Label>> m_labels;
};

}
