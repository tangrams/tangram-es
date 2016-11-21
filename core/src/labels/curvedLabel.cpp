#include "curvedLabel.h"

#include "textLabels.h"
#include "gl/dynamicQuadMesh.h"
#include "style/textStyle.h"
#include "text/fontContext.h"
#include "util/geom.h"
#include "util/lineSampler.h"
#include "view/view.h"
#include "log.h"
#include "alfons/path/splinePath.h"
#include "alfons/path/ASPC.h"

namespace Tangram {

using namespace LabelProperty;
using namespace TextLabelProperty;

CurvedLabel::CurvedLabel(Label::Options _options, float _prio,
                         TextLabel::VertexAttributes _attrib, glm::vec2 _dim,
                         TextLabels& _labels, TextRange _textRanges, Align _preferedAlignment,
                         size_t _anchorPoint, const std::vector<glm::vec2>& _line)

    : Label({{}}, _dim, Label::Type::curved, _options),
      m_textLabels(_labels),
      m_textRanges(_textRanges),
      m_fontAttrib(_attrib),
      m_preferedAlignment(_preferedAlignment),
      m_anchorPoint(_anchorPoint),
      m_line(_line) {

    m_prio = _prio;
    m_options.repeatDistance = 0;
    m_screenAnchorPoint = _anchorPoint;

    applyAnchor(m_options.anchors[0]);
}

void CurvedLabel::applyAnchor(Anchor _anchor) {

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

bool CurvedLabel::updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState,
                                      ScreenTransform& _transform, bool _drawAllLabels) {

    bool clipped = false;

    LineSampler<ScreenTransform> sampler { _transform };

    //bool inside = false;
    bool inside = true;

    for (auto& p : m_line) {
        glm::vec2 sp = worldToScreenSpace(_mvp, glm::vec4(p, 0.0, 1.0),
                                          _viewState.viewportSize, clipped);

        if (clipped) { return false; }

        sampler.add(sp);

        if (!inside) {
            if ((sp.x >= 0 && sp.x <= _viewState.viewportSize.x) ||
                (sp.y >= 0 && sp.y <= _viewState.viewportSize.y)) {
                inside = true;
            }
        }
    }

    float length = sampler.sumLength();

    if (!inside || length < m_dim.x) {
        sampler.clearPoints();
        return false;
    }

#if 0
    //if (m_line.size() > 3) {
    auto& points = _transform;

    auto& path = _transform.scratchBuffer();

    float tol = 50000;
    auto gamma = alfons::GammaBSpline;

    //float minDist2 = 20*20;
    //auto gamma = alfons::GammaCatmullRom;

    alfons::ASPC aspc(gamma, path, tol);
    int size = points.size();

    aspc.segment(glm::vec2(points[0]),
                 glm::vec2(points[0]),
                 glm::vec2(points[0]),
                 glm::vec2(points[1]));

    aspc.segment(glm::vec2(points[0]),
                 glm::vec2(points[0]),
                 glm::vec2(points[1]),
                 glm::vec2(points[2]));

    for (int i = 0; i < size - 3; i++) {
        // TODO insert anchor here?
        aspc.segment(glm::vec2(points[i]),
                     glm::vec2(points[i + 1]),
                     glm::vec2(points[i + 2]),
                     glm::vec2(points[i + 3]));
    }

    aspc.segment(glm::vec2(points[size - 3]),
                 glm::vec2(points[size - 2]),
                 glm::vec2(points[size - 1]),
                 glm::vec2(points[size - 1]));

    aspc.segment(glm::vec2(points[size - 2]),
                 glm::vec2(points[size - 1]),
                 glm::vec2(points[size - 1]),
                 glm::vec2(points[size - 1]));

    _transform.clear();

    glm::vec2 anchor = sampler.point(m_anchorPoint);

    sampler.clearPoints();

    m_screenAnchorPoint = path.size()/2;
    float minDist = std::numeric_limits<float>::infinity();
    size_t curAnchor = 0;

    for (auto& sp : path) {
        float dist = glm::distance2(anchor, sp);
        if (dist < minDist) {
            minDist = dist;
            m_screenAnchorPoint = curAnchor;
        }

        sampler.add(sp);
        curAnchor++;
    }
    length = sampler.sumLength();
#endif

    return true;
}

void CurvedLabel::obbs(ScreenTransform& _transform, std::vector<OBB>& _obbs,
                     Range& _range, bool _append) {

    // Dont update curved label ranges for now (This isn't used anyway)
    _append = true;

    _range.start = int(_obbs.size());

    glm::vec2 dim = m_dim - m_options.buffer;

    if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

    // TODO: Remove - Only for testing
    if (state() == State::dead) { dim -= 4; }

    float width = dim.x;
    LineSampler<ScreenTransform> sampler { _transform };

    auto center = sampler.point(m_screenAnchorPoint).z;
    //auto center = sampler.sumLength() * 0.5;

    auto start = center - width * 0.5f;

    glm::vec2 p1, p2, rotation;
    sampler.sample(start, p1, rotation);

    float prevLength = start;

    int count = 0;
    for (size_t i = sampler.curSegment()+1; i < _transform.size(); i++) {

        float currLength = sampler.point(i).z;
        float segmentLength = currLength - prevLength;
        count++;

        if (start + width > currLength) {
            p2 = glm::vec2(sampler.point(i));

            rotation = sampler.segmentDirection(i-1);
            _obbs.push_back({(p1 + p2) * 0.5f, rotation, segmentLength, dim.y});
            prevLength = currLength;
            p1 = p2;

        } else {

            segmentLength = (start + width) - prevLength;
            sampler.sample(start + width, p2, rotation);
            _obbs.push_back({(p1 + p2) * 0.5f, rotation, segmentLength, dim.y});
            break;
        }
    }
    _range.length = count;
}

void CurvedLabel::addVerticesToMesh(ScreenTransform& _transform) {
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

    glm::vec2 rotation;
    LineSampler<ScreenTransform> sampler { _transform };

    float width = m_dim.x;

    if (sampler.sumLength() < width) { return; }

    float center = sampler.point(m_screenAnchorPoint).z;
    //auto center = sampler.sumLength() * 0.5;

    glm::vec2 p1, p2;
    sampler.sample(center + it->quad[0].pos.x / TextVertex::position_scale, p1, rotation);
    // Check based on first charater whether labels needs to be flipped
    // sampler.sample(center + it->quad[2].pos.x, p2, rotation);
    sampler.sample(center + (end-1)->quad[2].pos.x / TextVertex::position_scale, p2, rotation);


    if (p1.x > p2.x) {
        sampler.reversePoints();
        center = sampler.sumLength() - center;
    }

    for (; it != end; ++it) {
        auto quad = *it;

        // auto xStart = quad.quad[0].pos.x / TextVertex::position_scale;
        // auto xEnd = quad.quad[2].pos.x / TextVertex::position_scale;

        glm::vec2 origin = {(quad.quad[0].pos.x + quad.quad[2].pos.x) * 0.5f, 0 };

        glm::vec2 point; //, pa, pb, ra, rb;
        float px = origin.x / TextVertex::position_scale;

        if (!sampler.sample(center + px, point, rotation)) {
            // break;
        }
        // bool ok1 = sampler.sample(center + xStart, pa, ra);
        // size_t s1 = sampler.curSegment();
        // bool ok2 = sampler.sample(center + xEnd, pb, rb);
        // size_t s2 = sampler.curSegment();

        // if (s1 != s2 && ok1 && ok2) {
        //     auto c = glm::vec2(sampler.point((s1 < s2 ? s1 : s2) + 1));
        //     auto da = glm::distance(c, pa);
        //     auto db = glm::distance(c, pb);
        //     float f = da / (da + db);
        //     rotation = glm::normalize(ra * f + rb * (1.0f - f));
        //     point =  (point + pa + pb) * 0.333333f;
        // }

        point *= TextVertex::position_scale;
        rotation = {rotation.x, -rotation.y};

        auto* quadVertices = meshes[it->atlas]->pushQuad();

        for (int i = 0; i < 4; i++) {
            TextVertex& v = quadVertices[i];

            v.pos = glm::i16vec2{point + rotateBy(glm::vec2(quad.quad[i].pos) - origin, rotation)};

            v.uv = quad.quad[i].uv;
            v.state = state;
        }
    }
}

}
