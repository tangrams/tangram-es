#include "label.h"

#include "util/geom.h"
#include "glm/gtx/rotate_vector.hpp"
#include "tangram.h"
#include "platform.h"

namespace Tangram {

Label::Label(Label::Transform _transform, glm::vec2 _size, Type _type, Options _options, LabelProperty::Anchor _anchor)
    : m_type(_type),
      m_transform(_transform),
      m_dim(_size),
      m_options(_options),
      m_anchorType(_anchor) {

    if (!m_options.collide || m_type == Type::debug){
        enterState(State::visible, 1.0);
    } else {
        m_state = State::wait_occ;
        m_transform.state.alpha = 0.0;
    }

    m_occludedLastFrame = false;
    m_occluded = false;
    m_parent = nullptr;
}

Label::~Label() {}

bool Label::updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize, bool _testVisibility) {

    glm::vec2 screenPosition;
    glm::vec2 rotation = {1, 0};
    bool clipped = false;

    switch (m_type) {
        case Type::debug:
        case Type::point:
        {
            glm::vec2 p0 = m_transform.modelPosition1;

            screenPosition = worldToScreenSpace(_mvp, glm::vec4(p0, 0.0, 1.0),
                                                _screenSize, clipped);

            if (_testVisibility && clipped) {
                return false;
            }

            screenPosition += m_anchor;

            break;
        }
        case Type::line:
        {
            // project label position from mercator world space to screen
            // coordinates
            glm::vec2 p0 = m_transform.modelPosition1;
            glm::vec2 p2 = m_transform.modelPosition2;

            glm::vec2 ap0 = worldToScreenSpace(_mvp, glm::vec4(p0, 0.0, 1.0),
                                               _screenSize, clipped);
            glm::vec2 ap2 = worldToScreenSpace(_mvp, glm::vec4(p2, 0.0, 1.0),
                                               _screenSize, clipped);

            m_transform.screenPosition1 = ap0;
            m_transform.screenPosition2 = ap2;

            // check whether the label is behind the camera using the
            // perspective division factor
            if (_testVisibility && clipped) {
                return false;
            }

            float length = glm::length(ap2 - ap0);

            // default heuristic : allow label to be 30% wider than segment
            float minLength = m_dim.x * 0.7;

            if (_testVisibility && length < minLength) {
                return false;
            }

            glm::vec2 p1 = glm::vec2(p2 + p0) * 0.5f;

            glm::vec2 ap1 = worldToScreenSpace(_mvp, glm::vec4(p1, 0.0, 1.0),
                                               _screenSize, clipped);

            // Keep screen position center at world center (less sliding in tilted view)
            screenPosition = ap1;

            rotation = (ap0.x <= ap2.x ? ap2 - ap0 : ap0 - ap2) / length;

            break;
        }
    }

    glm::vec2 offset = rotateBy(m_options.offset, rotation);

    m_transform.state.screenPos = screenPosition + offset;
    m_transform.state.rotation = rotation;

    return true;
}

void Label::setParent(const Label& _parent, bool _definePriority) {
    m_parent = &_parent;

    glm::vec2 anchorDir = LabelProperty::anchorDirection(_parent.anchorType());
    glm::vec2 anchorOrigin = anchorDir * _parent.dimension() * 0.5f;
    applyAnchor(m_dim + _parent.dimension(), anchorOrigin, m_anchorType);

    if (_definePriority) {
        m_options.priority = _parent.options().priority + 0.5f;
    }

    m_options.offset += _parent.options().offset;
}

bool Label::offViewport(const glm::vec2& _screenSize) {
    const auto& quad = m_obb.getQuad();

    for (int i = 0; i < 4; ++i) {
        const auto& p = quad[i];
        if (p.x < _screenSize.x && p.y < _screenSize.y && p.x > 0 && p.y > 0) {
            return false;
        }
    }

    return true;
}

bool Label::canOcclude() {
    if (!m_options.collide) {
        return false;
    }

    int occludeFlags = (State::visible |
                        State::wait_occ |
                        State::skip_transition |
                        State::fading_in |
                        State::sleep |
                        State::out_of_screen |
                        State::dead);

    return (occludeFlags & m_state) && !(m_type == Type::debug);
}

bool Label::visibleState() const {
    int visibleFlags = (State::visible |
                        State::fading_in |
                        State::fading_out |
                        State::skip_transition);

    return (visibleFlags & m_state);
}

void Label::skipTransitions() {
    enterState(State::skip_transition, 0.0);
}

glm::vec2 Label::center() const {
    return m_obb.getCentroid();
}

void Label::enterState(const State& _state, float _alpha) {
    if (m_state == State::dead) { return; }

    m_state = _state;
    setAlpha(_alpha);
}

void Label::setAlpha(float _alpha) {
    m_transform.state.alpha = CLAMP(_alpha, 0.0, 1.0);
}

void Label::resetState() {
    if (m_state == State::dead) { return; }

    m_occludedLastFrame = false;
    m_occluded = false;
    enterState(State::wait_occ, 0.0);
}

bool Label::update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _zoomFract, bool _allLabels) {

    m_occludedLastFrame = m_occluded;
    m_occluded = false;

    if (m_state == State::dead) {
        if (!_allLabels) {
            return false;
        } else {
            m_occluded = true;
        }
    }

    bool ruleSatisfied = updateScreenTransform(_mvp, _screenSize, !_allLabels);

    // one of the label rules has not been satisfied
    if (!ruleSatisfied) {
        enterState(State::sleep, 0.0);
        return false;
    }

    // update the view-space bouding box
    updateBBoxes(_zoomFract);

    // checks whether the label is out of the viewport
    if (offViewport(_screenSize)) {
        enterState(State::out_of_screen, 0.0);
    } else if (m_state == State::out_of_screen) {
        m_occludedLastFrame = true;
        enterState(State::sleep, 0.0);
    }

    return true;
}

bool Label::evalState(const glm::vec2& _screenSize, float _dt) {

    // if (Tangram::getDebugFlag(DebugFlags::all_labels)) {
    //     enterState(State::visible, 1.0);
    //     return false;
    // }

    bool animate = false;

    switch (m_state) {
        case State::visible:
            if (m_occluded) {
                m_fade = FadeEffect(false, m_options.hideTransition.ease,
                                    m_options.hideTransition.time);
                enterState(State::fading_out, 1.0);
                animate = true;
            }
            break;
        case State::fading_in:
            if (m_occluded) {
                enterState(State::sleep, 0.0);
                // enterState(State::fading_out, m_transform.state.alpha);
                // animate = true;
                break;
            }
            setAlpha(m_fade.update(_dt));
            animate = true;
            if (m_fade.isFinished()) {
                enterState(State::visible, 1.0);
            }
            break;
        case State::fading_out:
            if (!m_occluded) {
                enterState(State::fading_in, m_transform.state.alpha);
                animate = true;
                break;
            }
            setAlpha(m_fade.update(_dt));
            animate = true;
            if (m_fade.isFinished()) {
                enterState(State::sleep, 0.0);
            }
            break;
        case State::wait_occ:
            if (m_occluded) {
                enterState(State::sleep, 0.0);
            } else {
                m_fade = FadeEffect(true, m_options.showTransition.ease,
                                    m_options.showTransition.time);
                enterState(State::fading_in, 0.0);
                animate = true;
            }
            break;
        case State::skip_transition:
            if (m_occluded) {
                enterState(State::sleep, 0.0);
            } else {
                enterState(State::visible, 1.0);
            }
            break;
        case State::sleep:
            if (!m_occluded) {
                m_fade = FadeEffect(true, m_options.showTransition.ease,
                                    m_options.showTransition.time);
                enterState(State::fading_in, 0.0);
                animate = true;
            }
            break;
        case State::out_of_screen:
        case State::dead:
            break;
    }

    return animate;
}

}
