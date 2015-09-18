#pragma once

#include "labels/label.h"
#include "text/textBuffer.h"

namespace Tangram {

class TextLabel : public Label {

public:
    TextLabel(std::string _text, Label::Transform _transform, Type _type,
              glm::vec2 _dim, TextBuffer& _mesh, Range _vertexRange, Label::Options _options);
    
    const std::string& getText() const { return m_text; }

private:

    std::string m_text;


protected:

    void updateBBoxes() override;
};

}
