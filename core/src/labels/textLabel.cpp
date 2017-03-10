#include "labels/textLabel.h"

#include "gl/dynamicQuadMesh.h"
#include "labels/obbBuffer.h"
#include "labels/textLabels.h"
#include "labels/screenTransform.h"
#include "log.h"
#include "style/textStyle.h"
#include "text/fontContext.h"
#include "util/geom.h"
#include "view/view.h"

#include "glm/gtx/norm.hpp"

namespace Tangram {

using namespace LabelProperty;
using namespace TextLabelProperty;

const float TextVertex::position_scale = 4.0f;
const float TextVertex::position_inv_scale = 0.25f;
const float TextVertex::alpha_scale = 65535.0f;

struct PointTransform {
    ScreenTransform& m_transform;

    PointTransform(ScreenTransform& _transform)
        : m_transform(_transform) {}

    void set(glm::vec2 _position, glm::vec2 _rotation) {
        m_transform.push_back(_position);
        m_transform.push_back(_rotation);
    }

    glm::vec2 position() const { return glm::vec2(m_transform[0]); }
    glm::vec2 rotation() const { return glm::vec2(m_transform[1]); }
};

TextLabel::TextLabel(Coordinates _coordinates, Type _type, Label::Options _options,
                     TextLabel::VertexAttributes _attrib, glm::vec2 _dim,
                     TextLabels& _labels, TextRange _textRanges, Align _preferedAlignment)
    : Label(_dim, _type, _options),
      m_coordinates(_coordinates),
      m_textLabels(_labels),
      m_textRanges(_textRanges),
      m_fontAttrib(_attrib),
      m_preferedAlignment(_preferedAlignment) {

    applyAnchor(m_options.anchors[0]);
}

void TextLabel::applyAnchor(Anchor _anchor) {

    if (m_preferedAlignment == Align::none) {
        Align newAlignment = alignFromAnchor(_anchor);
        m_textRangeIndex = int(newAlignment);
    } else {
        m_textRangeIndex = int(m_preferedAlignment);
    }

    if (m_textRanges[m_textRangeIndex].length == 0) {
        m_textRangeIndex = 0;
    }

    glm::vec2 offset = m_dim;
    if (m_parent) { offset += m_parent->dimension(); }

    m_anchor = LabelProperty::anchorDirection(_anchor) * offset * 0.5f;
}

bool TextLabel::updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState,
                                      const AABB* _bounds, ScreenTransform& _transform) {

    bool clipped = false;

    switch(m_type) {
        case Type::debug:
        case Type::point: {

            glm::vec2 p0 = m_coordinates[0];

            glm::vec2 screenPosition = worldToScreenSpace(_mvp, glm::vec4(p0, 0.0, 1.0),
                                                          _viewState.viewportSize, clipped);

            if (clipped) { return false; }

            if (_bounds) {
                auto aabb = m_options.anchors.extents(m_dim);
                aabb.min += screenPosition + m_options.offset;
                aabb.max += screenPosition + m_options.offset;
                if (!aabb.intersect(*_bounds)) { return false; }
            }

            m_screenCenter = screenPosition;

            PointTransform(_transform).set(screenPosition + m_options.offset, glm::vec2{1, 0});

            return true;
        }
        case Type::line: {

            glm::vec2 rotation = {1, 0};

            // project label position from mercator world space to screen
            // coordinates
            glm::vec2 p0 = m_coordinates[0];
            glm::vec2 p2 = m_coordinates[1];

            glm::vec2 ap0 = worldToScreenSpace(_mvp, glm::vec4(p0, 0.0, 1.0),
                                               _viewState.viewportSize, clipped);
            glm::vec2 ap2 = worldToScreenSpace(_mvp, glm::vec4(p2, 0.0, 1.0),
                                               _viewState.viewportSize, clipped);

            // check whether the label is behind the camera using the
            // perspective division factor
            if (clipped) { return false; }

            if (_bounds) {
                AABB aabb;
                aabb.include(ap0.x, ap0.y);
                aabb.include(ap2.x, ap2.y);
                if (!aabb.intersect(*_bounds)) { return false; }
            }

            float length = glm::length(ap2 - ap0);

            // default heuristic : allow label to be 30% wider than segment
            float minLength = m_dim.x * 0.7;

            if (length < minLength) { return false; }

            glm::vec2 p1 = glm::vec2(p2 + p0) * 0.5f;

            // Keep screen position center at world center (less sliding in tilted view)
            glm::vec2 screenPosition = worldToScreenSpace(_mvp, glm::vec4(p1, 0.0, 1.0),
                                                          _viewState.viewportSize, clipped);

            rotation = (ap0.x <= ap2.x ? ap2 - ap0 : ap0 - ap2) / length;
            rotation = glm::vec2{ rotation.x, - rotation.y };

            m_screenCenter = screenPosition;

            PointTransform(_transform).set(screenPosition + rotateBy(m_options.offset, rotation), rotation);

            return true;
        }
        default:
            break;
    }

    return false;
}

float TextLabel::candidatePriority() const {
    if (m_type != Type::line) { return 0.f; }

    return 1.f / (glm::length2(m_coordinates[0] - m_coordinates[1]));
}

void TextLabel::obbs(ScreenTransform& _transform, OBBBuffer& _obbs) {

    glm::vec2 dim = m_dim;

    if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

    PointTransform pointTransform(_transform);
    auto rotation = pointTransform.rotation();

    auto obb = OBB(pointTransform.position() + m_anchor,
                   glm::vec2{rotation.x, -rotation.y},
                   dim.x, dim.y);

    _obbs.append(obb);

}

void TextLabel::addVerticesToMesh(ScreenTransform& _transform, const glm::vec2& _screenSize) {
    if (!visibleState()) { return; }

    TextVertex::State state {
        m_fontAttrib.selectionColor,
        m_fontAttrib.fill,
        m_fontAttrib.stroke,
        uint16_t(m_alpha * TextVertex::alpha_scale),
        uint16_t(m_fontAttrib.fontScale),
    };

    auto it = m_textLabels.quads.begin() + m_textRanges[m_textRangeIndex].start;
    auto end = it + m_textRanges[m_textRangeIndex].length;
    auto& style = m_textLabels.style;

    auto& meshes = style.getMeshes();

    PointTransform transform(_transform);

    glm::vec2 rotation = transform.rotation();
    bool rotate = (rotation.x != 1.f);

    glm::vec2 screenPosition = transform.position();
    screenPosition += m_anchor;
    glm::i16vec2 sp = glm::i16vec2(screenPosition * TextVertex::position_scale);
    std::array<glm::i16vec2, 4> vertexPosition;

    // Expand screen bounding box by text height
    // TODO: Better approximation.
    glm::i16vec2 min(-m_dim.y * TextVertex::position_scale);
    glm::i16vec2 max((_screenSize + m_dim.y) * TextVertex::position_scale);

    for (; it != end; ++it) {
        auto quad = *it;
        bool visible = false;

        if (rotate) {
            for (int i = 0; i < 4; i++) {
                vertexPosition[i] = sp + glm::i16vec2{rotateBy(quad.quad[i].pos, rotation)};
            }
        } else {
            for (int i = 0; i < 4; i++) {
                vertexPosition[i] = sp + quad.quad[i].pos;
            }
        }

        for (int i = 0; i < 4; i++) {
            if (!visible &&
                vertexPosition[i].x > min.x &&
                vertexPosition[i].x < max.x &&
                vertexPosition[i].y > min.y &&
                vertexPosition[i].y < max.y) {
                visible = true;
            }
        }
        if (!visible) { continue; }

        auto* quadVertices = meshes[it->atlas]->pushQuad();

        for (int i = 0; i < 4; i++) {
            TextVertex& v = quadVertices[i];
            v.pos = vertexPosition[i];
            v.uv = quad.quad[i].uv;
            v.state = state;
        }
    }
}

TextLabels::~TextLabels() {
    style.context()->releaseAtlas(m_atlasRefs);
}

void TextLabels::setQuads(std::vector<GlyphQuad>&& _quads, std::bitset<FontContext::max_textures> _atlasRefs) {
    quads = std::move(_quads);
    m_atlasRefs = _atlasRefs;

}

}
