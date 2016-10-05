#include "labels/spriteLabel.h"
#include "gl/dynamicQuadMesh.h"
#include "scene/spriteAtlas.h"
#include "style/pointStyle.h"
#include "log.h"

namespace Tangram {

using namespace LabelProperty;

const float SpriteVertex::position_scale = 4.0f;
const float SpriteVertex::alpha_scale = 65535.0f;
const float SpriteVertex::texture_scale = 65535.0f;

SpriteLabel::SpriteLabel(Label::Transform _transform, glm::vec2 _size, Label::Options _options,
                         float _extrudeScale, Texture* _texture, SpriteLabels& _labels, size_t _labelsPos)
    : Label(_transform, _size, Label::Type::point, _options),
      m_labels(_labels),
      m_labelsPos(_labelsPos),
      m_texture(_texture),
      m_extrudeScale(_extrudeScale) {

    applyAnchor(m_options.anchors[0]);
}

void SpriteLabel::applyAnchor(LabelProperty::Anchor _anchor) {

    m_anchor = LabelProperty::anchorDirection(_anchor) * m_dim * 0.5f;
}

void SpriteLabel::updateBBoxes(float _zoomFract, bool _occluded) {
    glm::vec2 sp = m_transform.state.screenPos;
    glm::vec2 dim = m_dim + glm::vec2(m_extrudeScale * 2.f * _zoomFract);

    if (_occluded) { dim += Label::activation_distance_threshold; }

    m_obb = OBB(sp, m_transform.state.rotation, dim.x, dim.y);
}

void SpriteLabel::pushTransform() {

    if (!visibleState()) { return; }

    // TODO
    // if (a_extrude.x != 0.0) {
    //     float dz = u_map_position.z - abs(u_tile_origin.z);
    //     vertex_pos.xy += clamp(dz, 0.0, 1.0) * UNPACK_EXTRUDE(a_extrude.xy);
    // }

    auto& quad = m_labels.quads[m_labelsPos];

    SpriteVertex::State state {
        quad.color,
        uint16_t(m_transform.state.alpha * SpriteVertex::alpha_scale),
        0,
    };

    auto& style = m_labels.m_style;

    // Before pushing our geometry to the mesh, we push the texture that should be
    // used to draw this label. We check a few potential textures in order of priority.
    Texture* tex = nullptr;
    if (m_texture) { tex = m_texture; }
    else if (style.texture()) { tex = style.texture().get(); }
    else if (style.spriteAtlas()) { tex = style.spriteAtlas()->texture(); }

    // If tex is null, the mesh will use the default point texture.
    style.getMesh()->pushTexture(tex);

    auto* quadVertices = style.getMesh()->pushQuad();

    glm::i16vec2 sp = glm::i16vec2(m_transform.state.screenPos * SpriteVertex::position_scale);

    for (int i = 0; i < 4; i++) {
        SpriteVertex& v = quadVertices[i];
        v.pos = sp + quad.quad[i].pos;
        v.uv = quad.quad[i].uv;
        //v.extrude = quad.quad[i].extrude;
        v.state = state;
    }
}

}
