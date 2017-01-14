#include "labels/spriteLabel.h"

#include "gl/dynamicQuadMesh.h"
#include "scene/spriteAtlas.h"
#include "style/pointStyle.h"
#include "view/view.h"
#include "log.h"

namespace Tangram {

using namespace LabelProperty;

const float SpriteVertex::alpha_scale = 65535.0f;
const float SpriteVertex::texture_scale = 65535.0f;

SpriteLabel::SpriteLabel(Label::WorldTransform _transform, glm::vec2 _size, Label::Options _options,
                         SpriteLabel::VertexAttributes _attrib, Texture* _texture,
                         SpriteLabels& _labels, size_t _labelsPos)
    : Label(_transform, _size, Label::Type::point, _options),
      m_labels(_labels),
      m_labelsPos(_labelsPos),
      m_texture(_texture),
      m_vertexAttrib(_attrib) {

    applyAnchor(m_options.anchors[0]);
}

void SpriteLabel::applyAnchor(LabelProperty::Anchor _anchor) {

    m_anchor = LabelProperty::anchorDirection(_anchor) * m_dim * 0.5f;
}

bool SpriteLabel::updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState, bool _drawAllLabels) {

    // NOTE: _viewState is constructed in labelCollider during tile building (does not match view parameters
    glm::vec2 halfScreen = glm::vec2(_viewState.viewportSize * 0.5f);

    switch (m_type) {
        case Type::debug:
        case Type::point:
        {
            glm::vec2 p0 = glm::vec2(m_worldTransform.position);

            if (m_options.flat) {

                auto& positions = m_screenTransform.positions;
                float sourceScale = pow(2, m_worldTransform.position.z);
                float scale = float(sourceScale / (_viewState.zoomScale * _viewState.tileSize * 2.0));
                float zoomFactor = 0.;
                if (_viewState.fractZoom < 1.f) { // Only need to apply during updates and not during tile building
                    zoomFactor = m_vertexAttrib.extrudeScale * scale * _viewState.fractZoom;
                }

                glm::vec2 dim = m_dim * scale + glm::vec2(zoomFactor, zoomFactor);
                positions[0] = -dim;
                positions[1] = glm::vec2(dim.x, -dim.y);
                positions[2] = glm::vec2(-dim.x, dim.y);
                positions[3] = dim;

                // Rotate in clockwise order
                if (m_options.angle != 0.f) {
                    glm::vec2 rotation(cos(DEG_TO_RAD * m_options.angle),
                                       sin(DEG_TO_RAD * m_options.angle));

                    positions[0] = rotateBy(positions[0], rotation);
                    positions[1] = rotateBy(positions[1], rotation);
                    positions[2] = rotateBy(positions[2], rotation);
                    positions[3] = rotateBy(positions[3], rotation);
                }

                for (size_t i = 0; i < 4; i++) {

                    positions[i] += p0;

                    glm::vec4 projected = worldToClipSpace(_mvp, glm::vec4(positions[i], 0.f, 1.f));
                    if (projected.w <= 0.0f) { return false; }

                        m_projected[i] = glm::vec3(projected) / projected.w;

                        // from normalized device coordinates to screen space coordinate system
                        // top-left screen axis, y pointing down
                        positions[i].x = 1 + m_projected[i].x;
                        positions[i].y = 1 - m_projected[i].y;
                        positions[i] *= halfScreen;
                }

            } else {
                glm::vec4 projected = worldToClipSpace(_mvp, glm::vec4(p0, 0.f, 1.f));
                if (projected.w <= 0.0f) { return false; }

                m_projected[0] = glm::vec3(projected) / projected.w;

                auto& position = m_screenTransform.position;
                position.x = 1 + m_projected[0].x;
                position.y = 1 - m_projected[0].y;
                position *= halfScreen;
                position += m_options.offset;

                m_projected[1].x = _viewState.viewportSize.x;
                m_projected[1].y = _viewState.viewportSize.y;
            }

            break;
        }
        default:
            break;
    }

    return true;
}

void SpriteLabel::updateBBoxes(float _zoomFract) {
    glm::vec2 dim;

    if (m_options.flat) {
        static float infinity = std::numeric_limits<float>::infinity();

        float minx = infinity, miny = infinity;
        float maxx = -infinity, maxy = -infinity;

        for (int i = 0; i < 4; ++i) {
            minx = std::min(minx, m_screenTransform.positions[i].x);
            miny = std::min(miny, m_screenTransform.positions[i].y);
            maxx = std::max(maxx, m_screenTransform.positions[i].x);
            maxy = std::max(maxy, m_screenTransform.positions[i].y);
        }

        dim = glm::vec2(maxx - minx, maxy - miny);

        if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

        // TODO: Manage extrude scale

        glm::vec2 obbCenter = glm::vec2((minx + maxx) * 0.5f, (miny + maxy) * 0.5f);

        m_obb = OBB(obbCenter, glm::vec2(1.0, 0.0), dim.x, dim.y);
    } else {
        dim = m_dim + glm::vec2(m_vertexAttrib.extrudeScale * _zoomFract);

        if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

        m_obb = OBB(m_screenTransform.position + m_anchor, m_screenTransform.rotation, dim.x, dim.y);
    }
}

void SpriteLabel::addVerticesToMesh() {

    if (!visibleState()) { return; }

    // TODO
    // if (a_extrude.x != 0.0) {
    //     float dz = u_map_position.z - abs(u_tile_origin.z);
    //     vertex_pos.xy += clamp(dz, 0.0, 1.0) * UNPACK_EXTRUDE(a_extrude.xy);
    // }

    auto& quad = m_labels.quads[m_labelsPos];

    SpriteVertex::State state {
        m_vertexAttrib.selectionColor,
        m_vertexAttrib.color,
        uint16_t(m_screenTransform.alpha * SpriteVertex::alpha_scale),
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

    if (m_options.flat) {
        for (int i = 0; i < 4; i++) {
            SpriteVertex& vertex = quadVertices[i];

            vertex.pos = m_projected[i];
            vertex.uv = quad.quad[i].uv;
            vertex.state = state;
        }

    } else {

        glm::vec2 pos = glm::vec2(m_projected[0]);
        glm::vec2 scale = 2.0f / glm::vec2(m_projected[1]);
        scale.y *= -1;

        pos += m_options.offset * scale;
        pos += m_anchor * scale;

        for (int i = 0; i < 4; i++) {
            SpriteVertex& vertex = quadVertices[i];

            glm::vec2 coord = pos + quad.quad[i].pos * scale;

            vertex.pos.x = coord.x;
            vertex.pos.y = coord.y;
            vertex.pos.z = 0;

            vertex.uv = quad.quad[i].uv;
            vertex.state = state;
        }
    }
}

}
