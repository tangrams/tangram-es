#include "labels/spriteLabel.h"
#include "labels/labelMesh.h"
#include "platform.h"

namespace Tangram {

using namespace LabelProperty;

SpriteLabel::SpriteLabel(Label::Transform _transform, glm::vec2 _size, LabelMesh& _mesh, int _vertexOffset,
        Label::Options _options, float _extrudeScale, LabelProperty::Anchor _anchor) :
    Label(_transform, _size, Label::Type::point, {_vertexOffset, 4}, _options),
    m_mesh(_mesh),
    m_extrudeScale(_extrudeScale)
{
    switch(_anchor) {
        case Anchor::left: m_anchor = glm::vec2(1.0, 0.5); break;
        case Anchor::right: m_anchor = glm::vec2(0.0, 0.5); break;
        case Anchor::top: m_anchor = glm::vec2(0.5, 0.0); break;
        case Anchor::bottom: m_anchor = glm::vec2(0.5, 1.0); break;
        case Anchor::bottom_left: m_anchor = glm::vec2(1.0, 1.0); break;
        case Anchor::bottom_right: m_anchor = glm::vec2(0.0, 1.0); break;
        case Anchor::top_left: m_anchor = glm::vec2(1.0, 0.0); break;
        case Anchor::top_right: m_anchor = glm::vec2(0.0, 0.0); break;
        case Anchor::center: m_anchor = glm::vec2(0.5); break;
    }
}

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

void SpriteLabel::pushTransform() {
    // update the buffer on valid states
    if (m_dirty) {
        static size_t attribOffset = offsetof(Label::Vertex, state);
        static size_t alphaOffset = offsetof(Label::Vertex::State, alpha) + attribOffset;

        if (visibleState()) {
            // update the complete state on the mesh
            m_mesh.updateAttribute(m_vertexRange, m_transform.state.vertex(), m_mesh.data(), attribOffset);
        } else {

            // for any non-visible states, we don't need to overhead the gpu with updates on the
            // alpha attribute, but simply do it once until the label goes back in a visible state
            if (m_updateMeshVisibility) {
                m_mesh.updateAttribute(m_vertexRange, (m_transform.state.vertex().alpha), m_mesh.data(), alphaOffset);
                m_updateMeshVisibility = false;
            }
        }

        m_dirty = false;
    }
}

}
