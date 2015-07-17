#pragma once

#include "label.h"
#include "text/textBuffer.h"

class TextLabel : public Label {

public:
    TextLabel(Label::Transform _transform, std::string _text, fsuint _id, Type _type);
    
    /* Call the font context to rasterize the label string */
    bool rasterize(TextBuffer& _buffer);
    
    std::string getText() { return m_text; }
    
    void pushTransform(VboMesh& _mesh) override;
    
private:
    
    std::string m_text;
    fsuint m_id;
    
protected:
    
    void updateBBoxes() override;
};