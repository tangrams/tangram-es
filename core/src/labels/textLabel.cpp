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
                     LabelProperty::Anchor _anchor, TextLabel::FontVertexAttributes _attrib,
                     glm::vec2 _dim,  TextLabels& _labels, Range _vertexRange)
    : Label(_transform, _dim, _type, _options),
      m_textLabels(_labels),
      m_vertexRange(_vertexRange),
      m_fontAttrib(_attrib) {

    m_anchor = glm::vec2(0);
    float width = _dim.x;
    float height = _dim.y;

    switch(_anchor) {
    case Anchor::center:
        break;
    case Anchor::left:
        m_anchor.x -= 0.5 * width;
        break;
    case Anchor::right:
        m_anchor.x += 0.5 * width;
        break;
    case Anchor::bottom:
        m_anchor.y += 0.5 * height;
        break;
    case Anchor::bottom_left:
        m_anchor.x -= 0.5 * width;
        m_anchor.y += 0.5 * height;
        break;
    case Anchor::bottom_right:
        m_anchor.x += 0.5 * width;
        m_anchor.y += 0.5 * height;
        break;
    case Anchor::top:
        m_anchor.y -= 0.5 * height;
        break;
    case Anchor::top_left:
        m_anchor.x -= 0.5 * width;
        m_anchor.y -= 0.5 * height;
        break;
    case Anchor::top_right:
        m_anchor.x += 0.5 * width;
        m_anchor.y -= 0.5 * height;
        break;
    }
}

void TextLabel::updateBBoxes(float _zoomFract) {

    m_obb = OBB(m_transform.state.screenPos.x,
                m_transform.state.screenPos.y,
                m_transform.state.rotation,
                m_dim.x - m_options.buffer,
                m_dim.y - m_options.buffer);

    m_aabb = m_obb.getExtent();
}

void TextLabel::align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) {

    switch (m_type) {
        case Type::debug:
        case Type::point:
            _screenPosition += m_anchor;
            break;
        case Type::line: {
            // anchor at line center
            _screenPosition = (_ap1 + _ap2) * 0.5f;
            break;
        }
    }
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

void TextLabels::setQuads(std::vector<GlyphQuad>& _quads) {
    quads.insert(quads.end(), _quads.begin(), _quads.end());

    for (auto& q : quads) {
        m_atlasRefs.set(q.atlas);
    }

    style.context()->lockAtlas(m_atlasRefs);
}

}
