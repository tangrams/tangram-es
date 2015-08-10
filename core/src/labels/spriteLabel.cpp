#include "labels/spriteLabel.h"

namespace Tangram {

SpriteLabel::SpriteLabel(LabelMesh& _mesh, Label::Transform _transform, const glm::vec2& _size, size_t _bufferOffset) :
    Label(_transform, _mesh, Label::Type::point, _bufferOffset, 4)
{
    m_dim = _size;
}

void SpriteLabel::updateBBoxes() {
    glm::vec2 sp = m_transform.state.screenPos;
    m_obb = isect2d::OBB(sp.x, sp.y, m_transform.state.rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}

}
