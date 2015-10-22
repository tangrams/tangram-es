#include "labels/spriteLabel.h"
#include "platform.h"

namespace Tangram {

SpriteLabel::SpriteLabel(Label::Transform _transform, glm::vec2 _size, LabelMesh& _mesh,
    int _vertexOffset, Label::Options _options, float _extrudeScale, glm::vec2 _anchor) :
    Label(_transform, _size, Label::Type::point, _mesh, {_vertexOffset, 4}, _options),
    m_extrudeScale(_extrudeScale)
{}

void SpriteLabel::updateBBoxes(float _zoomFract) {
    glm::vec2 halfSize = m_dim * 0.5f;
    glm::vec2 sp = m_transform.state.screenPos;
    glm::vec2 dim = m_dim + glm::vec2(m_extrudeScale * 2.f * _zoomFract);
    m_obb = OBB(sp.x + halfSize.x, sp.y - halfSize.y, m_transform.state.rotation, dim.x, dim.y);
    m_aabb = m_obb.getExtent();
}

void SpriteLabel::align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) {

    switch (m_type) {
        case Type::debug:
        case Type::point:
            _screenPosition.x -= m_dim.x * m_anchor.x;
            _screenPosition.y += m_dim.y * m_anchor.y;
            break;
        case Type::line:
            LOGW("Line sprite labels not implemented yet");
            break;
    }

}

}
