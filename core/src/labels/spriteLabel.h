#pragma once

#include "labels/label.h"

namespace Tangram {

class SpriteLabel : public Label {
public:

    SpriteLabel(LabelMesh& _mesh, Label::Transform _transform, const glm::vec2& _size, size_t _bufferOffset);

protected:

    void updateBBoxes() override;

};

}
