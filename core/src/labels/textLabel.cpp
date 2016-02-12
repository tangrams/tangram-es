#include "textLabel.h"

namespace Tangram {

using namespace LabelProperty;

TextLabel::TextLabel(Label::Transform _transform, Type _type,
                     Label::Options _options, Anchor _anchor,
                     TextLabel::FontVertexAttributes _attrib,
                     glm::vec2 _dim, FontMetrics _metrics,
                     int _nLines, glm::vec2 _quadsLocalOrigin,
                     TextLabels& _labels, Range _vertexRange)
    : Label(_transform, _dim, _type, _options),
      m_metrics(_metrics),
      m_nLines(_nLines),
      m_textLabels(_labels),
      m_vertexRange(_vertexRange),
      m_quadLocalOrigin(_quadsLocalOrigin),
      m_fontAttrib(_attrib) {

    if (m_type == Type::point) {
        glm::vec2 halfDim = m_dim * 0.5f;
        switch(_anchor) {
            case Anchor::left: m_anchor.x -= halfDim.x; break;
            case Anchor::right: m_anchor.x += halfDim.x; break;
            case Anchor::top: m_anchor.y -= halfDim.y; break;
            case Anchor::bottom: m_anchor.y += halfDim.y; break;
            case Anchor::bottom_left: m_anchor += glm::vec2(-halfDim.x, halfDim.y); break;
            case Anchor::bottom_right: m_anchor += halfDim; break;
            case Anchor::top_left: m_anchor -= halfDim; break;
            case Anchor::top_right: m_anchor += glm::vec2(halfDim.x, -halfDim.y); break;
            case Anchor::center: break;
        }
    }
}

void TextLabel::updateBBoxes(float _zoomFract) {
    glm::vec2 obbCenter;

    if (m_type == Type::line) {
        m_xAxis = glm::vec2(cos(m_transform.state.rotation), sin(m_transform.state.rotation));
        m_yAxis = glm::vec2(-m_xAxis.y, m_xAxis.x);
    }

    obbCenter = m_transform.state.screenPos;
    // move forward on line by half the text length
    obbCenter += m_xAxis * m_dim.x * 0.5f;
    // move down on the perpendicular to estimated font baseline
    obbCenter += m_yAxis * m_dim.y * 0.5f;
    // ajdust with local origin of the quads
    obbCenter += m_yAxis * m_quadLocalOrigin.y;
    obbCenter += m_xAxis * m_quadLocalOrigin.x;

    m_obb = OBB(obbCenter.x, obbCenter.y, m_transform.state.rotation,
                m_dim.x + m_options.buffer, m_dim.y + m_options.buffer);
    m_aabb = m_obb.getExtent();
}

void TextLabel::align(glm::vec2& _screenPosition, const glm::vec2& _ap1, const glm::vec2& _ap2) {

    switch (m_type) {
        case Type::debug:
        case Type::point:
            // modify position set by updateScreenTransform()
            _screenPosition.x -= m_dim.x * 0.5f + m_quadLocalOrigin.x;
            _screenPosition.y -= m_metrics.descender;

            if (m_nLines > 1) {
                _screenPosition.y -= m_dim.y * 0.5f;
                _screenPosition.y += (m_metrics.lineHeight + m_metrics.descender);
            }
            _screenPosition += m_anchor;
            break;
        case Type::line: {
            // anchor at line center
            _screenPosition = (_ap1 + _ap2) * 0.5f;

            // move back by half the length (so that text will be drawn centered)
            glm::vec2 direction = glm::normalize(_ap1 - _ap2);
            _screenPosition += direction * m_dim.x * 0.5f;
            _screenPosition += m_yAxis * (m_dim.y * 0.5f + m_metrics.descender);
            break;
        }
    }
}

void TextLabel::pushTransform() {
    if (!visibleState()) { return; }

    Label::Vertex::State state {
        glm::i16vec2(m_transform.state.screenPos * position_scale),
        uint8_t(m_transform.state.alpha * alpha_scale),
        uint8_t(m_fontAttrib.fontScale),
        int16_t(m_transform.state.rotation * rotation_scale)
    };

    auto it = m_textLabels.quads.begin() + m_vertexRange.start;
    auto end = it + m_vertexRange.length;
    auto& style = m_textLabels.m_style;

    for (; it != end; ++it) {
        auto quad = *it;

        auto* quadVertices = style.mesh(it->atlas).pushQuad();
        for (int i = 0; i < 4; i++) {
            Label::Vertex& v = quadVertices[i];
            v.pos = quad.quad[i].pos;
            v.uv = quad.quad[i].uv;
            v.color = m_fontAttrib.fill;
            v.stroke = m_fontAttrib.stroke;
            v.state = state;
        }
    }
}


}
