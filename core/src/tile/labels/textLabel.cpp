#include "textLabel.h"

TextLabel::TextLabel(Label::Transform _transform, std::string _text, fsuint _id, Type _type) :
    Label(_transform, _type),
    m_text(_text),
    m_id(_id) {
    
}

bool TextLabel::rasterize(std::shared_ptr<TextBuffer>& _buffer) {
    bool res = _buffer->rasterize(m_text, m_id);
    
    if (!res) {
        return false;
    }
    
    glm::vec4 bbox = _buffer->getBBox(m_id);
    
    m_dim.x = std::abs(bbox.z - bbox.x);
    m_dim.y = std::abs(bbox.w - bbox.y);
    
    return true;
}

void TextLabel::pushTransform() {
    if (m_dirty) {
        // TODO : get the buffer
        //_buffer->transformID(m_id, m_transform.m_screenPosition.x, m_transform.m_screenPosition.y, m_transform.m_rotation, m_transform.m_alpha);
        m_dirty = false;
    }
}