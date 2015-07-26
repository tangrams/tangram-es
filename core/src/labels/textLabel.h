#pragma once

#include "label.h"
#include "text/textBuffer.h"

namespace Tangram {

class TextLabel : public Label {

public:
    TextLabel(Label::Transform _transform, std::string _text, Type _type);
    
    /* Call the font context to rasterize the label string */
    bool rasterize(TextBuffer& _buffer);
    
    std::string getText() { return m_text; }
    
    void pushTransform(VboMesh& _mesh) override;
    
private:
    
    std::string m_text;

    size_t m_bufferOffset;
    int m_numGlyphs;
    
protected:
    
    void updateBBoxes() override;
};

}
