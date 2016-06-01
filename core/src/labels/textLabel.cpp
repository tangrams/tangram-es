#include "textLabel.h"
#include "textLabels.h"
#include "style/textStyle.h"
#include "text/fontContext.h"
#include "gl/dynamicQuadMesh.h"

namespace Tangram {

using namespace LabelProperty;

const float TextVertex::position_scale = 4.0f;
const float TextVertex::rotation_scale = 4096.0f;
const float TextVertex::alpha_scale = 255.f;

TextLabel::TextLabel(Label::Transform _transform, Type _type, Label::Options _options,
                     Anchor _anchor, TextLabel::FontVertexAttributes _attrib,
                     glm::vec2 _dim,  TextLabels& _labels, Range _vertexRange)
    : Label(_transform, _dim, _type, _options, _anchor),
      m_textLabels(_labels),
      m_vertexRange(_vertexRange),
      m_fontAttrib(_attrib) {

    applyAnchor(_dim, glm::vec2(0.0), _anchor);
}

void TextLabel::applyAnchor(const glm::vec2& _dimension, const glm::vec2& _origin, Anchor _anchor) {
    m_anchor = _origin + LabelProperty::anchorDirection(_anchor) * _dimension * 0.5f;
}

void TextLabel::updateBBoxes(float _zoomFract) {

    glm::vec2 dim = m_dim - m_options.buffer;

    if (m_occludedLastFrame) { dim += 2; }

    m_obb = OBB(m_transform.state.screenPos.x,
                m_transform.state.screenPos.y,
                m_transform.state.rotation,
                dim.x, dim.y);
}

void TextLabel::pushTransform() {
    if (!visibleState()) { return; }

    TextVertex::State state {
        m_fontAttrib.fill,
        m_fontAttrib.stroke,
        glm::i16vec2(m_transform.state.screenPos * TextVertex::position_scale),
        uint8_t(m_transform.state.alpha * TextVertex::alpha_scale),
        uint8_t(m_fontAttrib.fontScale),
        int16_t(m_transform.state.rotation * TextVertex::rotation_scale)
    };

    auto it = m_textLabels.quads.begin() + m_vertexRange.start;
    auto end = it + m_vertexRange.length;
    auto& style = m_textLabels.style;

    for (; it != end; ++it) {
        auto quad = *it;

        auto* quadVertices = style.getMesh(it->atlas).pushQuad();
        for (int i = 0; i < 4; i++) {
            TextVertex& v = quadVertices[i];
            v.pos = quad.quad[i].pos;
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
