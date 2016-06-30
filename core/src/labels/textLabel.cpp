#include "textLabel.h"
#include "textLabels.h"
#include "style/textStyle.h"
#include "text/fontContext.h"
#include "gl/dynamicQuadMesh.h"
#include "util/geom.h"

namespace Tangram {

using namespace LabelProperty;
using namespace TextLabelProperty;

const float TextVertex::position_scale = 4.0f;
const float TextVertex::rotation_scale = 4096.0f;
const float TextVertex::alpha_scale = 255.f;

TextLabel::TextLabel(Label::Transform _transform, Type _type, Label::Options _options,
                     Anchor _anchor, TextLabel::FontVertexAttributes _attrib,
                     glm::vec2 _dim,  TextLabels& _labels, std::vector<TextRange> _textRanges,
                     Align _preferedAlignment)
    : Label(_transform, _dim, _type, _options, _anchor),
      m_textLabels(_labels),
      m_textRanges(_textRanges),
      m_fontAttrib(_attrib),
      m_preferedAlignment(_preferedAlignment) {

    applyAnchor(_dim, glm::vec2(0.0), _anchor);
    m_textRangeIndex = 0;
}

void TextLabel::applyAnchor(const glm::vec2& _dimension, const glm::vec2& _origin, Anchor _anchor) {

    if (m_preferedAlignment == Align::none) {
        Align newAlignment = alignFromAnchor(_anchor);
        for (int i = 0; i < m_textRanges.size(); ++i) {
            if (m_textRanges[i].align == newAlignment) {
                m_textRangeIndex = i;
                break;
            }
        }
    }

    m_anchor = _origin + LabelProperty::anchorDirection(_anchor) * _dimension * 0.5f;
}

void TextLabel::updateBBoxes(float _zoomFract) {

    glm::vec2 dim = m_dim - m_options.buffer;

    if (m_occludedLastFrame) { dim += 2; }

    // FIXME: Only for testing
    if (state() == State::dead) { dim -= 4; }

    m_obb = OBB(m_transform.state.screenPos,
                m_transform.state.rotation,
                dim.x, dim.y);
}

void TextLabel::pushTransform() {
    if (!visibleState()) { return; }

    bool rotate = (m_transform.state.rotation.x != 1.f);
    glm::vec2 rotation = {m_transform.state.rotation.x, -m_transform.state.rotation.y};

    TextVertex::State state {
        m_fontAttrib.fill,
        m_fontAttrib.stroke,
        uint8_t(m_transform.state.alpha * TextVertex::alpha_scale),
        uint8_t(m_fontAttrib.fontScale),
        0
    };

    auto it = m_textLabels.quads.begin() + m_textRanges[m_textRangeIndex].range.start;
    auto end = it + m_textRanges[m_textRangeIndex].range.length;
    auto& style = m_textLabels.style;

    glm::i16vec2 sp = glm::i16vec2(m_transform.state.screenPos * TextVertex::position_scale);

    for (; it != end; ++it) {
        auto quad = *it;

        auto* quadVertices = style.getMesh(it->atlas).pushQuad();
        for (int i = 0; i < 4; i++) {
            TextVertex& v = quadVertices[i];
            if (rotate) {
                v.pos = sp + glm::i16vec2{rotateBy(quad.quad[i].pos, rotation)};
            } else {
                v.pos = sp + quad.quad[i].pos;
            }
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
