#include "spriteLabel.h"
#include "style/spriteStyle.h"

SpriteLabel::SpriteLabel(Label::Transform _transform, const glm::vec2& _size, const glm::vec2& _offset, size_t _bufferOffset) :
    Label(_transform, Label::Type::point),
    m_offset(_offset),
    m_bufferOffset(_bufferOffset) {

    m_dim = _size;
}

void SpriteLabel::pushTransform(Batch& _batch) {

    if (m_dirty) {
        if (typeid(_batch) != typeid(SpriteBatch))
            return;
        
        auto& batch = static_cast<SpriteBatch&>(_batch);
        batch.m_mesh->updateAttribute(m_bufferOffset, 4, m_transform.state);
    }
}

void SpriteLabel::updateBBoxes() {
    glm::vec2 sp = m_transform.state.screenPos + m_offset;
    m_obb = isect2d::OBB(sp.x, sp.y, m_transform.state.rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}
