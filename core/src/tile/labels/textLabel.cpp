#include "textLabel.h"

TextLabel::TextLabel(Label::Transform _transform, std::string _text, fsuint _id, Type _type) :
    Label(_transform, _type),
    m_text(_text),
    m_id(_id)
{}

void TextLabel::updateBBoxes() {
    glm::vec2 t = glm::vec2(cos(m_transform.state.rotation), sin(m_transform.state.rotation));
    glm::vec2 tperp = glm::vec2(-t.y, t.x);
    glm::vec2 obbCenter;
    
    obbCenter = m_transform.state.screenPos + t * m_dim.x * 0.5f - tperp * (m_dim.y / 8);
    
    m_obb = isect2d::OBB(obbCenter.x, obbCenter.y, m_transform.state.rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}

bool TextLabel::rasterize(TextBuffer& _buffer) {
    bool res = _buffer.rasterize(m_text, m_id);
    
    if (!res) {
        return false;
    }
    
    glm::vec4 bbox = _buffer.getBBox(m_id);
    
    m_dim.x = std::abs(bbox.z - bbox.x);
    m_dim.y = std::abs(bbox.w - bbox.y);
    
    return true;
}

void TextLabel::pushTransform(VboMesh& _mesh) {
    if (m_dirty) {
        TextBuffer& buffer = static_cast<TextBuffer&>(_mesh);
        buffer.transformID(m_id, m_transform.state.screenPos.x,
                           m_transform.state.screenPos.y,
                           m_transform.state.rotation,
                           m_transform.state.alpha);
        m_dirty = false;
    }
}
