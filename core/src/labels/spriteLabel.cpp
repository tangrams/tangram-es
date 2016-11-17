#include "labels/spriteLabel.h"

#include "gl/dynamicQuadMesh.h"
#include "scene/spriteAtlas.h"
#include "style/pointStyle.h"
#include "view/view.h"
#include "util/geom.h"
#include "log.h"

namespace Tangram {

using namespace LabelProperty;

const float SpriteVertex::alpha_scale = 65535.0f;
const float SpriteVertex::texture_scale = 65535.0f;

struct BillboardTransform {
    ScreenTransform& m_transform;

    BillboardTransform(ScreenTransform& _transform)
        : m_transform(_transform) {}

    void set(glm::vec2 _position, glm::vec3 _projected, glm::vec2 _screenSize) {

        m_transform.push_back(_position);
        m_transform.push_back(_projected);
        m_transform.push_back(_screenSize);
    }

    glm::vec2 position() { return glm::vec2(m_transform[0]); }
    glm::vec3 projected() { return m_transform[1]; }
    glm::vec2 screenSize() { return glm::vec2(m_transform[2]); }
};

struct FlatTransform {
    ScreenTransform& m_transform;

    FlatTransform(ScreenTransform& _transform)
        : m_transform(_transform) {}

    void set(const std::array<glm::vec2, 4>& _position,
             const std::array<glm::vec3, 4>& _projected) {
        for (size_t i = 0; i < 4; i++) {
            m_transform.push_back(_position[i]);
        }
        for (size_t i = 0; i < 4; i++) {
            m_transform.push_back(_projected[i]);
        }
    }

    glm::vec2 position(size_t i) { return glm::vec2(m_transform[i]); }
    glm::vec3 projected(size_t i) { return m_transform[4+i]; }
};

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

bool SpriteLabel::updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState,
                                        ScreenTransform& _transform, bool _drawAllLabels) {

    glm::vec2 halfScreen = glm::vec2(_viewState.viewportSize * 0.5f);

    switch (m_type) {
        case Type::debug:
        case Type::point:
        {
            glm::vec2 p0 = glm::vec2(m_worldTransform.position);

            if (m_options.flat) {

                std::array<glm::vec2, 4> positions;
                std::array<glm::vec3, 4> projected;

                float sourceScale = pow(2, m_worldTransform.position.z);
                float scale = float(sourceScale / (_viewState.zoomScale * _viewState.tileSize * 2.0));
                if (m_vertexAttrib.extrudeScale != 1.f) {
                    scale *= pow(2, _viewState.fractZoom) * m_vertexAttrib.extrudeScale;
                }
                glm::vec2 dim = m_dim * scale;

                positions[0] = p0 - dim;
                positions[1] = p0 + glm::vec2(dim.x, -dim.y);
                positions[2] = p0 + glm::vec2(-dim.x, dim.y);
                positions[3] = p0 + dim;

                // Rotate in clockwise order on the ground plane
                if (m_options.angle != 0.f) {
                    glm::vec2 rotation(cos(DEG_TO_RAD * m_options.angle),
                                       sin(DEG_TO_RAD * m_options.angle));

                    for (size_t i = 0; i < 4; i++) {
                        positions[i] = rotateBy(positions[i], rotation);
                    }
                }

                for (size_t i = 0; i < 4; i++) {
                    glm::vec4 proj = worldToClipSpace(_mvp, glm::vec4(positions[i], 0.f, 1.f));
                    if (proj.w <= 0.0f) { return false; }

                    projected[i] = glm::vec3(proj) / proj.w;

                    // from normalized device coordinates to screen space coordinate system
                    // top-left screen axis, y pointing down
                    positions[i].x = 1 + projected[i].x;
                    positions[i].y = 1 - projected[i].y;
                    positions[i] *= halfScreen;
                }

                FlatTransform(_transform).set(positions, projected);

            } else {
                glm::vec4 projected = worldToClipSpace(_mvp, glm::vec4(p0, 0.f, 1.f));
                if (projected.w <= 0.0f) { return false; }

                projected /=  projected.w;

                glm::vec2 position;
                position.x = 1 + projected.x;
                position.y = 1 - projected.y;
                position *= halfScreen;
                position += m_options.offset;

                BillboardTransform(_transform).set(position, glm::vec3(projected),
                                                   _viewState.viewportSize);
            }
            break;
        }
        default:
            break;
    }

    return true;
}

void SpriteLabel::obbs(ScreenTransform& _transform, std::vector<OBB>& _obbs,
                       Range& _range, bool _append) {

    if (_append) { _range.start = int(_obbs.size()); }
    _range.length = 1;

    glm::vec2 dim = m_dim + glm::vec2(m_vertexAttrib.extrudeScale * 2.f); // * _zoomFract);

    OBB obb;

    if (m_options.flat) {
        static float infinity = std::numeric_limits<float>::infinity();

        float minx = infinity, miny = infinity;
        float maxx = -infinity, maxy = -infinity;

        for (int i = 0; i < 4; ++i) {

            const auto& position = _transform[i];
            minx = std::min(minx, position.x);
            miny = std::min(miny, position.y);
            maxx = std::max(maxx, position.x);
            maxy = std::max(maxy, position.y);
        }

        dim = glm::vec2(maxx - minx, maxy - miny);

        if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

        // TODO: Manage extrude scale

        glm::vec2 obbCenter = glm::vec2((minx + maxx) * 0.5f, (miny + maxy) * 0.5f);

        obb = OBB(obbCenter, glm::vec2(1.0, 0.0), dim.x, dim.y);
    } else {

        if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

        BillboardTransform pointTransform(_transform);

        obb = OBB(pointTransform.position() + m_anchor, {1,0}, dim.x, dim.y);
    }

    if (_append) {
        _obbs.push_back(obb);
    } else {
        _obbs[_range.start] = obb;
    }
}

void SpriteLabel::addVerticesToMesh(ScreenTransform& _transform) {

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
        uint16_t(m_alpha * SpriteVertex::alpha_scale),
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
        FlatTransform transform(_transform);

        for (int i = 0; i < 4; i++) {
            SpriteVertex& vertex = quadVertices[i];

            vertex.pos = transform.projected(i);
            vertex.uv = quad.quad[i].uv;
            vertex.state = state;
        }

    } else {

        BillboardTransform transform(_transform);

        glm::vec2 pos = glm::vec2(transform.projected());
        glm::vec2 scale = 2.0f / transform.screenSize();
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
