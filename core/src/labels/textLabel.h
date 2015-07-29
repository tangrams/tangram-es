#pragma once

#include "labels/label.h"
#include "text/textBuffer.h"

namespace Tangram {

class TextLabel : public Label {

public:
    TextLabel(std::string _text, Label::Transform _transform, Type _type, int _numGlyphs,
              glm::vec2 _dim, TextBuffer& _mesh, size_t _bufferOffset);
    
    std::string getText() { return m_text; }

private:

    std::string m_text;


protected:

    void updateBBoxes() override;
};

}
