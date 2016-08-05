#include "label.h"

#include "util/geom.h"
#include "glm/gtx/rotate_vector.hpp"
#include "tangram.h"
#include "platform.h"

namespace Tangram {

const float Label::activation_distance_threshold = 2;

Label::Label(Label::Transform _transform, glm::vec2 _size, Type _type, Options _options)
    : m_state(State::none),
      m_type(_type),
      m_transform(_transform),
      m_dim(_size),
      m_options(_options) {

    if (!m_options.collide || m_type == Type::debug){
        enterState(State::visible, 1.0);
    } else {
        m_transform.state.alpha = 0.0;
    }

    m_occludedLastFrame = false;
    m_occluded = false;
    m_parent = nullptr;
    m_anchorIndex = 0;
}

Label::~Label() {}

bool Label::updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize, bool _drawAllLabels) {

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

            if (clipped) {
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
                                               _screenSize, clipped);

            // Keep screen position center at world center (less sliding in tilted view)
            screenPosition = ap1;

            rotation = (ap0.x <= ap2.x ? ap2 - ap0 : ap0 - ap2) / length;
            rotation = glm::vec2{rotation.x, -rotation.y};
            break;
        }
    }

    m_transform.state.screenPos = screenPosition + rotateBy(m_options.offset, rotation);
    m_transform.state.rotation = rotation;

    return true;
}

void Label::alignFromParent(const Label& _parent) {
    glm::vec2 anchorDir = LabelProperty::anchorDirection(_parent.anchorType());
    glm::vec2 anchorOrigin = anchorDir * _parent.dimension() * 0.5f;

    applyAnchor(m_dim + _parent.dimension(), anchorOrigin, m_options.anchors[m_anchorIndex]);
    m_options.offset += _parent.options().offset;
}

void Label::setParent(const Label& _parent, bool _definePriority) {
    m_parent = &_parent;

    alignFromParent(_parent);

    if (_definePriority) {
        m_options.priority = _parent.options().priority + 0.5f;
    }
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
                        State::none |
                        State::skip_transition |
                        State::fading_in |
                        State::sleep |
                        State::anchor_fallback |
                        State::out_of_screen |
                        State::dead);

    return (occludeFlags & m_state) && !(m_type == Type::debug);
}

bool Label::visibleState() const {
    int visibleFlags = (State::visible |
                        State::fading_in |
                        State::fading_out |
                        State::anchor_fallback |
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

    if (m_state == State::sleep) {
        // Reset anchor fallback index
        m_anchorIndex = 0;
    }

    if (m_state == State::anchor_fallback) {
        m_anchorIndex = 1;
    }
}

void Label::setAlpha(float _alpha) {
    m_transform.state.alpha = CLAMP(_alpha, 0.0, 1.0);
}

void Label::resetState() {

    if (m_state == State::dead) { return; }

    m_occludedLastFrame = false;
    m_occluded = false;
    m_anchorIndex = 0;
    enterState(State::none, 0.0);
}

void Label::print() const {
    LOG("Label - %p", this);
    LOG("\tm_occludedLastFrame: %d", m_occludedLastFrame);
    LOG("\tm_occluded: %d", m_occluded);
    std::string state;
    switch (m_state) {
        case State::none: state = "none"; break;
        case State::visible: state = "visible"; break;
        case State::anchor_fallback: state = "anchor_fallback"; break;
        case State::fading_in: state = "fading_in"; break;
        case State::fading_out: state = "fading_out"; break;
        case State::skip_transition: state = "skip_transition"; break;
        case State::sleep: state = "sleep"; break;
        case State::dead: state = "dead"; break;
        case State::out_of_screen: state = "out_of_screen"; break;
    }
    LOG("\tm_state: %s", state.c_str());
    //LOG("\tm_options.anchorFallback: %s", m_options.anchorFallbacks.to_string().c_str());
}

bool Label::update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _zoomFract, bool _drawAllLabels) {

    m_occludedLastFrame = m_occluded;
    m_occluded = false;

    if (m_state == State::dead) {
        if (_drawAllLabels) {
            m_occluded = true;
        } else {
            return false;
        }
    }

    if (m_state == State::anchor_fallback) {
        // Apply new alignment
        if (m_parent) {
            alignFromParent(*m_parent);
        } else {
            applyAnchor(m_dim, glm::vec2(0.0), m_options.anchors[m_anchorIndex]);
        }
    }

    bool ruleSatisfied = updateScreenTransform(_mvp, _screenSize, _drawAllLabels);

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
        if (m_occludedLastFrame) {
            m_occluded = true;
            return false;
        }
    } else if (m_state == State::out_of_screen) {
        if (m_occludedLastFrame) {
            enterState(State::sleep, 0.0);
        } else {
            enterState(State::visible, 1.0);
        }
    }

    return true;
}

Label::EvalUpdate Label::evalState(float _dt) {

#ifdef DEBUG
    if (Tangram::getDebugFlag(DebugFlags::draw_all_labels)) {
        enterState(State::visible, 1.0);
        return EvalUpdate::none;
    }
#endif

    switch (m_state) {
        case State::visible:
            if (m_occluded) {
                if (m_options.anchors.count > 1) {
                    enterState(State::anchor_fallback, 0.0);

                    return EvalUpdate::relayout;
                } else {
                    m_fade.reset(false, m_options.hideTransition.ease,
                                        m_options.hideTransition.time);

                    enterState(State::fading_out, 1.0);
                    return EvalUpdate::animate;
                }

            } else if (m_anchorIndex != 0) {
                // TODO: try to place again to prefered fallback (0)
            }
            return EvalUpdate::none;

        case State::anchor_fallback:
            if (m_occluded) {
                if (m_anchorIndex >= int(m_options.anchors.count)-1) {
                    // Tried all anchors - deactivate label
                    enterState(State::sleep, 0.0);
                } else {
                    // Move to next one for upcoming frame
                    m_anchorIndex++;
                }
                return EvalUpdate::relayout;
            }
            enterState(State::visible, 1.0);
            return EvalUpdate::none;

        case State::fading_in:
            if (m_occluded) {
                enterState(State::sleep, 0.0);
                return EvalUpdate::none;
            }
            setAlpha(m_fade.update(_dt));
            if (m_fade.isFinished()) {
                enterState(State::visible, 1.0);
                return EvalUpdate::none;
            }
            return EvalUpdate::animate;

        case State::fading_out:
            if (!m_occluded) {
                enterState(State::fading_in, m_transform.state.alpha);
                return EvalUpdate::animate;
            }
            setAlpha(m_fade.update(_dt));
            if (m_fade.isFinished()) {
                enterState(State::sleep, 0.0);
                return EvalUpdate::none;
            }
            return EvalUpdate::animate;

        case State::none:
            if (m_occluded) {
                if (m_options.anchors.count > 1) {
                    enterState(State::anchor_fallback, 0.0);
                    return EvalUpdate::relayout;
                } else {
                    enterState(State::sleep, 0.0);
                    return EvalUpdate::none;
                }
            }
            m_fade.reset(true, m_options.showTransition.ease,
                         m_options.showTransition.time);
            enterState(State::fading_in, 0.0);
            return EvalUpdate::animate;

        case State::skip_transition:
            if (m_occluded) {
                enterState(State::sleep, 0.0);
            } else {
                enterState(State::visible, 1.0);
            }
            return EvalUpdate::none;

        case State::sleep:
            if (!m_occluded) {
                m_fade.reset(true, m_options.showTransition.ease,
                                   m_options.showTransition.time);

                enterState(State::fading_in, 0.0);
                return EvalUpdate::animate;
            }
            return EvalUpdate::none;

        case State::dead:
        case State::out_of_screen:
            break;
    }

    return EvalUpdate::none;
}

}
