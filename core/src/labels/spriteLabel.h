#pragma once

#include "labels/label.h"

namespace Tangram {

class SpriteLabel : public Label {
public:

    SpriteLabel(Label::Transform _transform, glm::vec2 _size, LabelMesh& _mesh, int _vertexOffset, Label::Options _options);

protected:

    void updateBBoxes() override;

};

}
