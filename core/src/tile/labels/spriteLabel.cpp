#include "spriteLabel.h"

SpriteLabel::SpriteLabel(Label::Transform _transform, const glm::vec2& _size, const glm::vec2& _offset, AttributeOffsets _attribOffsets) :
    Label(_transform, Label::Type::point),
    m_offset(_offset),
    m_attribOffsets(_attribOffsets) {

    m_dim = _size;
}

void SpriteLabel::pushTransform(VboMesh& _mesh) {
    if (m_dirty) {
        TypedMesh<BufferVert>& mesh = static_cast<TypedMesh<BufferVert>&>(_mesh);
        int memOffset = m_attribOffsets.memOffset;
        mesh.updateAttribute(m_attribOffsets.m_position + memOffset, 4, glm::vec2(m_transform.m_screenPosition.x, m_transform.m_screenPosition.y));
        mesh.updateAttribute(m_attribOffsets.m_alpha + memOffset, 4, m_transform.m_alpha);
        mesh.updateAttribute(m_attribOffsets.m_rotation + memOffset, 4, m_transform.m_rotation);
    }
}

void SpriteLabel::updateBBoxes() {
    glm::vec2 sp = m_transform.m_screenPosition + m_offset;
    m_obb = isect2d::OBB(sp.x, sp.y, m_transform.m_rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}
