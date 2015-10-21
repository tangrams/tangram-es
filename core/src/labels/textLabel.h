#pragma once

#include "labels/label.h"
#include "text/textBuffer.h"

namespace Tangram {

class TextLabel : public Label {

public:
    TextLabel(Label::Transform _transform, Type _type,
              glm::vec2 _dim, TextBuffer& _mesh, Range _vertexRange, Label::Options _options);

    void updateBBoxes(float _zoomFract) override;

};

}
