#include "spriteLabel.h"

SpriteLabel::SpriteLabel(Label::Transform _transform, const glm::vec2& _size, const glm::vec2& _offset, size_t _bufferOffset) :
    Label(_transform, Label::Type::point),
    m_offset(_offset),
    m_bufferOffset(_bufferOffset) {

    m_dim = _size;
}

void SpriteLabel::pushTransform(VboMesh& _mesh) {

    if (m_dirty) {
        TypedMesh<BufferVert>& mesh = static_cast<TypedMesh<BufferVert>&>(_mesh);

        mesh.updateAttribute(m_bufferOffset, 4, m_transform.state);
    }
}

void SpriteLabel::updateBBoxes() {
    glm::vec2 sp = m_transform.state.screenPos + m_offset;
    m_obb = isect2d::OBB(sp.x, sp.y, m_transform.state.rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}
