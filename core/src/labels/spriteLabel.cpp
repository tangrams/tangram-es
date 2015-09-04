#include "labels/spriteLabel.h"

namespace Tangram {

SpriteLabel::SpriteLabel(Label::Transform _transform, glm::vec2 _size,
                         LabelMesh& _mesh, int _vertexOffset, Label::Options _options) :
    Label(_transform, _size, Label::Type::point, _mesh, {_vertexOffset, 4}, _options) {}

void SpriteLabel::updateBBoxes() {
    glm::vec2 sp = m_transform.state.screenPos;
    m_obb = isect2d::OBB(sp.x, sp.y, m_transform.state.rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}

}
