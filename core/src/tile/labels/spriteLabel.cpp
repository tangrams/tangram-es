#include "spriteLabel.h"

SpriteLabel::SpriteLabel(Label::Transform _transform, const glm::vec2& _size, const glm::vec2& _offset, AttributeOffsets _attribOffsets) :
    Label(_transform, Label::Type::POINT),
    m_offset(_offset),
    m_attribOffsets(_attribOffsets) {
        
    m_dim = _size;
}

void SpriteLabel::pushTransform(VboMesh& _mesh) {
}

void SpriteLabel::updateBBoxes() {
    glm::vec2 sp = m_transform.m_screenPosition + m_offset;
    m_obb = isect2d::OBB(sp.x, sp.y, m_transform.m_rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}