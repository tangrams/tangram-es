#pragma once

#include "label.h"
#include "text/textBuffer.h"

class TextLabel : public Label {

public:
    TextLabel(Label::Transform _transform, std::string _text, fsuint _id, Type _type, std::weak_ptr<TextBuffer> _textBuffer);
    
    /* Call the font context to rasterize the label string */
    bool rasterize(std::shared_ptr<TextBuffer>& _buffer);
    
    std::string getText() { return m_text; }
    
    void pushTransform() override;
    
private:
    std::string m_text;
    fsuint m_id;
    std::weak_ptr<TextBuffer> m_textBuffer;
};