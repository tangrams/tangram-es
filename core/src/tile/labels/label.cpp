#include "label.h"

Label::Label(LabelTransform _transform, std::string _text, std::shared_ptr<FontContext> _fontContext, std::shared_ptr<TextBuffer> _buffer) :
    m_transform(_transform),
    m_text(_text),
    m_fontContext(_fontContext),
    m_buffer(_buffer) {
}

Label::~Label() {

}
