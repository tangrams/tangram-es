#pragma once

#include "labels/label.h"
#include "text/textBuffer.h"

namespace Tangram {

class TextLabel : public Label {

public:
    TextLabel(Label::Transform _transform, Type _type,
              glm::vec2 _dim, TextBuffer& _mesh, Range _vertexRange, Label::Options _options);

    void updateBBoxes(float _zoomFract) override;

protected:

    void align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) override;
};

}
