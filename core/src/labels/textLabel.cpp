#include "textLabel.h"

#include "textLabels.h"
#include "style/textStyle.h"
#include "text/fontContext.h"
#include "gl/dynamicQuadMesh.h"
#include "util/geom.h"
#include "view/view.h"
#include "log.h"

namespace Tangram {

using namespace LabelProperty;
using namespace TextLabelProperty;

const float TextVertex::position_scale = 4.0f;
const float TextVertex::alpha_scale = 65535.0f;

TextLabel::TextLabel(Label::WorldTransform _transform, Type _type, Label::Options _options,
                     TextLabel::FontVertexAttributes _attrib,
                     glm::vec2 _dim,  TextLabels& _labels, TextRange _textRanges,
                     Align _preferedAlignment)
    : Label(_transform, _dim, _type, _options),
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

bool TextLabel::updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState, bool _drawAllLabels) {
    bool clipped = false;

    switch (m_type) {
        case Type::debug:
        case Type::point:
        {
            glm::vec2 p0 = glm::vec2(m_worldTransform.position);

            glm::vec2 position = worldToScreenSpace(_mvp, glm::vec4(p0, 0.0, 1.0),
                                                    _viewState.viewportSize, clipped);
            if (clipped) { return false; }

            m_screenTransform.position = position + m_options.offset;

            break;
        }
        case Type::line:
        {
            // project label position from mercator world space to screen coordinates
            glm::vec2 p0 = m_worldTransform.positions[0];
            glm::vec2 p2 = m_worldTransform.positions[1];
            glm::vec2 position;

            glm::vec2 ap0 = worldToScreenSpace(_mvp, glm::vec4(p0, 0.0, 1.0),
                                               _viewState.viewportSize, clipped);
            glm::vec2 ap2 = worldToScreenSpace(_mvp, glm::vec4(p2, 0.0, 1.0),
                                               _viewState.viewportSize, clipped);

            // check whether the label is behind the camera using the
            // perspective division factor
            if (clipped) {
                return false;
            }

            float length = glm::length(ap2 - ap0);

            // default heuristic : allow label to be 30% wider than segment
            float minLength = m_dim.x * 0.7;

            if (!_drawAllLabels && length < minLength) {
                return false;
            }

            glm::vec2 p1 = glm::vec2(p2 + p0) * 0.5f;

            glm::vec2 ap1 = worldToScreenSpace(_mvp, glm::vec4(p1, 0.0, 1.0),
                                               _viewState.viewportSize, clipped);

            // Keep screen position center at world center (less sliding in tilted view)
            position = ap1;

            glm::vec2 rotation = (ap0.x <= ap2.x ? ap2 - ap0 : ap0 - ap2) / length;
            rotation = glm::vec2{rotation.x, -rotation.y};

            m_screenTransform.position = position + rotateBy(m_options.offset, rotation);
            m_screenTransform.rotation = rotation;

            break;
        }
    }

    return true;
}

void TextLabel::updateBBoxes(float _zoomFract) {

    glm::vec2 dim = m_dim - m_options.buffer;

    if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

    // FIXME: Only for testing
    if (state() == State::dead) { dim -= 4; }

    glm::vec2 screenPosition = m_screenTransform.position;
    screenPosition += m_anchor;

    m_obb = OBB(screenPosition,
                glm::vec2(m_screenTransform.rotation.x, -m_screenTransform.rotation.y),
                dim.x, dim.y);
}

void TextLabel::addVerticesToMesh() {
    if (!visibleState()) { return; }

    glm::vec2 rotation = m_screenTransform.rotation;
    bool rotate = (rotation.x != 1.f);

    TextVertex::State state {
        m_fontAttrib.fill,
        m_fontAttrib.stroke,
        uint16_t(m_screenTransform.alpha * TextVertex::alpha_scale),
        uint16_t(m_fontAttrib.fontScale),
    };

    auto it = m_textLabels.quads.begin() + m_textRanges[m_textRangeIndex].start;
    auto end = it + m_textRanges[m_textRangeIndex].length;
    auto& style = m_textLabels.style;

    glm::vec2 screenPosition = m_screenTransform.position;
    screenPosition += m_anchor;

    glm::i16vec2 sp = glm::i16vec2(screenPosition * TextVertex::position_scale);
    auto& meshes = style.getMeshes();

    for (; it != end; ++it) {
        auto quad = *it;

        if (it->atlas >= meshes.size()) {
            LOGE("Accesing inconsistent quad mesh (id:%u, size:%u)",
                 it->atlas, meshes.size());
            break;
        }
        auto* quadVertices = meshes[it->atlas]->pushQuad();
        for (int i = 0; i < 4; i++) {
            TextVertex& v = quadVertices[i];
            if (rotate) {
                v.pos = sp + glm::i16vec2{rotateBy(quad.quad[i].pos, rotation)};
            } else {
                v.pos = sp + quad.quad[i].pos;
            }
            v.uv = quad.quad[i].uv;
            v.state = state;
            v.selection = options().selectionColor;
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
