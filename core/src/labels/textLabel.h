#pragma once

#include "labels/label.h"
#include "text/textBuffer.h"

namespace Tangram {

class TextLabel : public Label {

public:
    TextLabel(TextBuffer& _mesh, Label::Transform _transform, std::string _text, Type _type);

    /* Call the font context to rasterize the label string */
    bool rasterize();

    std::string getText() { return m_text; }

private:

    std::string m_text;

protected:

    void updateBBoxes() override;
};

}
