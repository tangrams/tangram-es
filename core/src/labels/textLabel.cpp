#include "labels/textLabel.h"

namespace Tangram {

TextLabel::TextLabel(TextBuffer& _mesh, Label::Transform _transform, std::string _text, Type _type) :
    Label(_transform, static_cast<LabelMesh&>(_mesh), _type, 0, 0),
    m_text(_text)
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

    auto& buffer = dynamic_cast<TextBuffer&>(m_mesh);
    int nGlyphs = buffer.rasterize(m_text, m_dim, m_bufferOffset);
    m_nVerts = nGlyphs * 6;

    if (nGlyphs == 0) {
        return false;
    }
    return true;
}

}
