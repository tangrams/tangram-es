#pragma once

#include "labels/label.h"
#include "text/textBuffer.h"

namespace Tangram {

class TextLabel : public Label {

public:
    TextLabel(TileID _tileID, std::string _text, Label::Transform _transform, Type _type,
              glm::vec2 _dim, TextBuffer& _mesh, Range _vertexRange, Label::Options _options);
    
    std::string getText() { return m_text; }

private:

    std::string m_text;


protected:

    void updateBBoxes() override;
};

}
