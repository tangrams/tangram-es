#include "labels/spriteLabel.h"

namespace Tangram {

SpriteLabel::SpriteLabel(Label::Transform _transform, glm::vec2 _size, LabelMesh& _mesh,
                         int _vertexOffset, Label::Options _options, float _extrudeScale) :
    Label(_transform, _size, Label::Type::point, _mesh, {_vertexOffset, 4}, _options),
    m_extrudeScale(_extrudeScale)
{}

void SpriteLabel::updateBBoxes(float _zoomFract) {
    glm::vec2 sp = m_transform.state.screenPos;
    glm::vec2 dim = m_dim + glm::vec2(m_extrudeScale * 2.f * _zoomFract);

    m_obb = OBB(sp.x, sp.y, m_transform.state.rotation, dim.x, dim.y);
    m_aabb = m_obb.getExtent();
}

}
