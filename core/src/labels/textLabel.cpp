#include "textLabel.h"

namespace Tangram {

TextLabel::TextLabel(TextBuffer& _mesh, Label::Transform _transform, std::string _text, Type _type) :
    Label(_transform, _type),
    m_text(_text),
    m_mesh(_mesh)
{}

void TextLabel::updateBBoxes() {
    glm::vec2 t = glm::vec2(cos(m_transform.state.rotation), sin(m_transform.state.rotation));
    glm::vec2 tperp = glm::vec2(-t.y, t.x);
    glm::vec2 obbCenter;

    obbCenter = m_transform.state.screenPos + t * m_dim.x * 0.5f - tperp * (m_dim.y / 8);

    m_obb = isect2d::OBB(obbCenter.x, obbCenter.y, m_transform.state.rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}

bool TextLabel::rasterize() {

    m_numGlyphs = m_mesh.rasterize(m_text, m_dim, m_bufferOffset);

    if (m_numGlyphs == 0) {
        return false;
    }
    return true;
}

void TextLabel::pushTransform() {
    if (m_dirty) {
        m_dirty = false;
        size_t attribOffset = offsetof(Label::Vertex, state);
        int numVerts = m_numGlyphs * 6;

        m_mesh.updateAttribute(m_bufferOffset + attribOffset, numVerts, m_transform.state);
    }
}

}
