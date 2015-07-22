#pragma once

#include "label.h"

namespace Tangram {

class SpriteLabel : public Label {
public:

    SpriteLabel(Label::Transform _transform, const glm::vec2& _size, size_t _bufferOffset);

    void pushTransform(VboMesh& _mesh) override;

protected:

    void updateBBoxes() override;

    size_t m_bufferOffset;
};

}
