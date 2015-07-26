#pragma once

#include "label.h"

namespace Tangram {

class SpriteLabel : public Label {
public:

    SpriteLabel(LabelMesh& _mesh, Label::Transform _transform, const glm::vec2& _size, size_t _bufferOffset);

    void pushTransform() override;

protected:

    void updateBBoxes() override;

    // Back-pointer to owning container
    LabelMesh& m_mesh;

    // byte-offset in m_mesh vertices
    size_t m_bufferOffset;
};

}
