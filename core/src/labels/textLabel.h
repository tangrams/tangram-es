#pragma once

#include "label.h"
#include "text/textBuffer.h"

namespace Tangram {

class TextLabel : public Label {

public:
    TextLabel(TextBuffer& _mesh, Label::Transform _transform, std::string _text, Type _type);
    
    /* Call the font context to rasterize the label string */
    bool rasterize();
    
    std::string getText() { return m_text; }
    
    void pushTransform() override;
    
private:
    
    std::string m_text;

    int m_numGlyphs;

    // Back-pointer to owning container
    TextBuffer& m_mesh;

    // byte-offset in m_mesh vertices
    size_t m_bufferOffset;

protected:
    
    void updateBBoxes() override;
};

}
