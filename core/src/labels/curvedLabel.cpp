#include "labels/curvedLabel.h"

#include "gl/dynamicQuadMesh.h"
#include "labels/obbBuffer.h"
#include "labels/screenTransform.h"
#include "log.h"
#include "style/textStyle.h"
#include "text/fontContext.h"
#include "textLabels.h"
#include "util/geom.h"
#include "util/lineSampler.h"
#include "view/view.h"

#include <glm/gtx/norm.hpp>

namespace Tangram {

void CurvedLabel::applyAnchor(LabelProperty::Anchor _anchor) {

    using namespace TextLabelProperty;

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
                                        const AABB* _bounds, ScreenTransform& _transform) {

    glm::vec2 min(-m_dim.y);
    glm::vec2 max(_viewState.viewportSize + m_dim.y);

    bool clipped = false;
    bool inside = false;

    LineSampler<ScreenTransform> sampler { _transform };

    for (auto& p : m_modelTransform) {
        glm::vec2 sp = worldToScreenSpace(_mvp, glm::vec4(p, 0.0, 1.0),
                                          _viewState.viewportSize, clipped);

        if (clipped) { return false; }

        sampler.add(sp);

        if (!inside) {
            if ((sp.x > min.x && sp.x < max.x &&
                 sp.y > min.y && sp.y < max.y)) {
                inside = true;
            }
        }
    }

    if (!inside || sampler.sumLength() < m_dim.x) {
        sampler.clearPoints();
        return false;
    }

    auto center = sampler.point(m_anchorPoint);

    // Set center for repeatGroup distance calculations
    m_screenCenter = glm::vec2(center);
    m_screenAnchorPoint = m_anchorPoint;

    // Chord length for minimal ~120 degree inner angles (squared)
    // sin(60)*2
    const float sqDirLimit = powf(1.7f, 2);
    // Range to check for angle changes
    const float sampleWindow = 20;

    float width = m_dim.x;
    float start = sampler.point(m_anchorPoint).z - width * 0.5f;

    if (start < 0 || start + width > sampler.sumLength()) {
        sampler.clearPoints();
        return false;
    }

    // Sets current segment to the segment on which the text starts.
    glm::vec2 startPos, r;
    sampler.sample(start, startPos, r);
    size_t startSegment = sampler.curSegment();

    glm::vec2 dir = sampler.segmentDirection(startSegment);

    // Cannot reverse the direction when some glyphs must be
    // placed in forward direction and vice versa.
    const float flipTolerance = sin(DEG_TO_RAD * 45);
    bool mustForward = dir.x > flipTolerance;
    bool mustReverse = dir.x < -flipTolerance;

    for (size_t i = startSegment + 1; i < _transform.size(); i++) {
        float currLength = sampler.point(i).z;
        dir = sampler.segmentDirection(i);

        if (dir.x > flipTolerance) {
            mustForward = true;
        }
        if (dir.x < -flipTolerance) {
            mustReverse = true;
        }

        if (mustForward && mustReverse) {
            sampler.clearPoints();
            return false;
        }

        // Go back within window to check for hard direction changes
        for (int k = i - 1; k >= int(startSegment); k--) {
            if (glm::length2(sampler.segmentDirection(k) + dir) < sqDirLimit) {
                sampler.clearPoints();
                return false;
            }
            if (sampler.point(k).z < sampler.point(i).z - sampleWindow) {
                break;
            }
        }
        if (currLength > start + width) {
            break;
        }
    }

    if (!mustReverse) {
        // TODO use better heuristic to decide flipping
        glm::vec2 endPos;
        sampler.sample(start + width, endPos, r);
        if (startPos.x > endPos.x) {
            mustReverse = true;
        }
    }

    if (mustReverse) {
        sampler.reversePoints();
        m_screenAnchorPoint = _transform.size() - m_anchorPoint - 1;
    }

    return true;
}

void CurvedLabel::obbs(ScreenTransform& _transform, OBBBuffer& _obbs) {

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
            _obbs.append({(p1 + p2) * 0.5f, rotation, segmentLength, dim.y});
            prevLength = currLength;
            p1 = p2;

        } else {

            segmentLength = (start + width) - prevLength;
            sampler.sample(start + width, p2, rotation);
            _obbs.append({(p1 + p2) * 0.5f, rotation, segmentLength, dim.y});
            break;
        }
    }
}

void CurvedLabel::addVerticesToMesh(ScreenTransform& _transform, const glm::vec2& _screenSize) {
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

    std::array<glm::i16vec2, 4> vertexPosition;

    glm::i16vec2 min(-m_dim.y * TextVertex::position_scale);
    glm::i16vec2 max((_screenSize + m_dim.y) * TextVertex::position_scale);

    for (; it != end; ++it) {
        auto quad = *it;

        glm::vec2 origin = {(quad.quad[0].pos.x + quad.quad[2].pos.x) * 0.5f, 0 };
        glm::vec2 point, p1, p2, r1, r2;

        auto xStart = quad.quad[0].pos.x * TextVertex::position_inv_scale;
        auto xEnd = quad.quad[2].pos.x * TextVertex::position_inv_scale;

        float px = origin.x * TextVertex::position_inv_scale;
        if (!sampler.sample(center + px, point, rotation)) {
            continue;
        }

        bool ok1 = sampler.sample(center + xStart, p1, r1);
        bool ok2 = sampler.sample(center + xEnd, p2, r2);
        if (ok1 && ok2) {
            if (r1 == r2) {
                rotation = r1;
            } else {
                rotation = glm::normalize(p2 - p1);
            }
            //point = (p1 + p2) * 0.5f;
            point = point * 0.5f + (p1 + p2) * 0.25f;
        }

        glm::i16vec2 p(point * TextVertex::position_scale);

        rotation = {rotation.x, -rotation.y};

        bool visible = false;

        for (int i = 0; i < 4; i++) {

            vertexPosition[i] = p + glm::i16vec2{rotateBy(glm::vec2(quad.quad[i].pos) - origin, rotation)};

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

}
