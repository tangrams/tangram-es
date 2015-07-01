#include "spriteLabel.h"

SpriteLabel::SpriteLabel(Label::Transform _transform) : Label(_transform, Label::Type::point) {
    // TODO : get size
    m_dim = {32, 32};
}

void SpriteLabel::pushTransform() {
    // TODO : update vbo mesh
}

void SpriteLabel::updateBBoxes() {
    glm::vec2 sp = m_transform.m_screenPosition;
    m_obb = isect2d::OBB(sp.x, sp.y, m_transform.m_rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}
