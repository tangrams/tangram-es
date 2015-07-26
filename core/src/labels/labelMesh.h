#pragma once

#include "gl/typedMesh.h"
//#include "labels/label.h"
#include "glm/vec2.hpp"

#include <memory>
#include <vector>

namespace Tangram {

class Label;

struct BufferVert {
    glm::vec2 pos;
    glm::vec2 uv;
    struct State {
        glm::vec2 screenPos;
        float alpha;
        float rotation;
    } state;
};

class LabelMesh : public TypedMesh<BufferVert> {
public:
    LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
        : TypedMesh<BufferVert>(_vertexLayout, _drawMode, GL_DYNAMIC_DRAW) {
    }

    void addLabel(std::shared_ptr<Label> _label) {
        m_labels.push_back(std::move(_label));
    }

    std::vector<std::shared_ptr<Label>>& getLabels() {
        return m_labels;
    }

protected:
    std::vector<std::shared_ptr<Label>> m_labels;
};

}
