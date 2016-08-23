#include "labels/spriteLabel.h"
#include "gl/dynamicQuadMesh.h"
#include "style/pointStyle.h"
#include "platform.h"

namespace Tangram {

using namespace LabelProperty;

const float SpriteVertex::position_scale = 4.0f;
const float SpriteVertex::alpha_scale = 65535.0f;
const float SpriteVertex::texture_scale = 65535.0f;

SpriteLabel::SpriteLabel(Label::Transform _transform, glm::vec2 _size, Label::Options _options,
                         float _extrudeScale, SpriteLabels& _labels, size_t _labelsPos)
    : Label(_transform, _size, Label::Type::point, _options),
      m_labels(_labels),
      m_labelsPos(_labelsPos),
      m_extrudeScale(_extrudeScale) {

    applyAnchor(m_options.anchors[0]);
}

void SpriteLabel::applyAnchor(LabelProperty::Anchor _anchor) {

    m_anchor = LabelProperty::anchorDirection(_anchor) * m_dim * 0.5f;
}

void SpriteLabel::updateBBoxes(float _zoomFract) {
    glm::vec2 dim;

    if (m_options.flat) {
        static float infinity = std::numeric_limits<float>::infinity();

        float minx = infinity, miny = infinity;
        float maxx = -infinity, maxy = -infinity;

        for (int i = 0; i < 4; ++i) {
            minx = std::min(minx, screenBillboard[i].x);
            miny = std::min(miny, screenBillboard[i].y);
            maxx = std::max(maxx, screenBillboard[i].x);
            maxy = std::max(maxy, screenBillboard[i].y);
        }

        dim = glm::vec2(maxx - minx, maxy - miny);

        if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

        // TODO: Manage extrude scale

        glm::vec2 obbCenter = glm::vec2((minx + maxx) * 0.5f, (miny + maxy) * 0.5f);

        m_obb = OBB(obbCenter, glm::vec2(1.0, 0.0), dim.x, dim.y);
    } else {
        dim = m_dim + glm::vec2(m_extrudeScale * 2.f * _zoomFract);

        if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

        m_obb = OBB(m_transform.state.screenPos, m_transform.state.rotation, dim.x, dim.y);
    }
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

    auto* quadVertices = style.getMesh()->pushQuad();

    glm::i16vec2 screenPosition = glm::i16vec2(m_transform.state.screenPos * SpriteVertex::position_scale);

    for (int i = 0; i < 4; i++) {
        SpriteVertex& vertex = quadVertices[i];

        if (m_options.flat) {
            vertex.pos = SpriteVertex::position_scale * screenBillboard[i];
        } else {
            vertex.pos = screenPosition + quad.quad[i].pos;
        }

        vertex.uv = quad.quad[i].uv;
        //v.extrude = quad.quad[i].extrude;
        vertex.state = state;
    }
}

}
