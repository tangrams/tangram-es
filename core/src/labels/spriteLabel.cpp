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

        // used to write all attributes memory at once, good for caching
        struct VertexAttributes {
            glm::vec2 screenPos;
            float alpha;
            float rot;
        };

        GLintptr stride = std::min<GLintptr>(m_attribOffsets.m_position, std::min<GLintptr>(m_attribOffsets.m_alpha, m_attribOffsets.m_rotation));

        VertexAttributes newAttributes {
            m_transform.m_screenPosition + m_offset,
            m_transform.m_alpha,
            m_transform.m_rotation,
        };

        mesh.updateAttribute(stride + m_attribOffsets.memOffset, 4, newAttributes);
    }
}

void SpriteLabel::updateBBoxes() {
    glm::vec2 sp = m_transform.m_screenPosition + m_offset;
    m_obb = isect2d::OBB(sp.x, sp.y, m_transform.m_rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}
