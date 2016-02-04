#include "label.h"

#include "util/geom.h"
#include "labels/labelMesh.h"
#include "glm/gtx/rotate_vector.hpp"

namespace Tangram {

Label::Label(Label::Transform _transform, glm::vec2 _size, Type _type, LabelMesh& _mesh,
             Range _vertexRange, Options _options) :
    m_type(_type),
    m_transform(_transform),
    m_dim(_size),
    m_mesh(_mesh),
    m_vertexRange(_vertexRange),
    m_options(_options) {
    if (!m_options.collide || m_type == Type::debug){
        enterState(State::visible, 1.0);
    } else {
        m_state = State::wait_occ;
        m_transform.state.alpha = 0.0;
    }

    m_occludedLastFrame = false;
    m_updateMeshVisibility = true;
    m_dirty = true;
    m_proxy = false;
    m_xAxis = glm::vec2(1.0, 0.0);
    m_yAxis = glm::vec2(0.0, 1.0);
}

Label::~Label() {}

void Label::setProxy(bool _proxy) {
    m_proxy = _proxy;
}

bool Label::updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize, bool _testVisibility) {

    glm::vec2 screenPosition;
    float rot = 0;

    glm::vec2 ap1, ap2;

    switch (m_type) {
        case Type::debug:
        case Type::point:
        {
            glm::vec4 v1 = worldToClipSpace(_mvp, glm::vec4(m_transform.modelPosition1, 0.0, 1.0));

            if (_testVisibility && (v1.w <= 0)) {
                return false;
            }

            screenPosition = clipToScreenSpace(v1, _screenSize);

            ap1 = ap2 = screenPosition;

            break;
        }
        case Type::line:
        {
            // project label position from mercator world space to clip
            // coordinates
            glm::vec4 v1 = worldToClipSpace(_mvp, glm::vec4(m_transform.modelPosition1, 0.0, 1.0));
            glm::vec4 v2 = worldToClipSpace(_mvp, glm::vec4(m_transform.modelPosition2, 0.0, 1.0));

            // check whether the label is behind the camera using the
            // perspective division factor
            if (_testVisibility && (v1.w <= 0 || v2.w <= 0)) {
                return false;
            }

            // project to screen space
            glm::vec2 p1 = clipToScreenSpace(v1, _screenSize);
            glm::vec2 p2 = clipToScreenSpace(v2, _screenSize);

            rot = angleBetweenPoints(p1, p2) + M_PI_2;

            if (rot > M_PI_2 || rot < -M_PI_2) { // un-readable labels
                rot += M_PI;
            } else {
                std::swap(p1, p2);
            }

            float length = glm::length(p2 - p1);

            float exceedHeuristic = 30; // default heuristic : 30%

            if (_testVisibility && (m_dim.x > length)) {
                float exceed = (1 - (length / m_dim.x)) * 100;
                if (exceed > exceedHeuristic) {
                    return false;
                }
            }

            ap1 = p1;
            ap2 = p2;

            break;
        }
    }

    align(screenPosition, ap1, ap2);

    // update screen position
    glm::vec2 offset = m_options.offset;

    if (m_transform.state.rotation != 0.f) {
        offset = glm::rotate(offset, m_transform.state.rotation);
    }

    glm::vec2 newScreenPos = screenPosition + offset;
    if (newScreenPos != m_transform.state.screenPos) {
        m_transform.state.screenPos = newScreenPos;
        m_dirty = true;
    }

    // update screen rotation
    if (m_transform.state.rotation != rot) {
        m_transform.state.rotation = rot;
        m_dirty = true;
    }

    return true;
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

void Label::occlude(bool _occlusion) {
    m_occluded = _occlusion;
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
                        State::out_of_screen);

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
    m_prevState = m_state;
    m_state = _state;
    setAlpha(_alpha);
}

void Label::setAlpha(float _alpha) {
    float alpha = CLAMP(_alpha, 0.0, 1.0);
    if (m_transform.state.alpha != alpha) {
        m_transform.state.alpha = alpha;
        m_dirty = true;
    }

    if (alpha == 0.f) {
        m_updateMeshVisibility = true;
    }
}

void Label::pushTransform() {

    // update the buffer on valid states
    if (m_dirty) {
        const static size_t attribOffset = offsetof(Label::Vertex, state);
        const static size_t alphaOffset = offsetof(Label::Vertex::State, alpha) + attribOffset;

        if (visibleState()) {
            // update the complete state on the mesh
            m_mesh.updateAttribute(m_vertexRange, m_transform.state.vertex(), attribOffset);
        } else {

            // for any non-visible states, we don't need to overhead the gpu
            // with updates on the alpha attribute, but simply do it once until
            // the label goes back in a visible state
            if (m_updateMeshVisibility) {
                m_mesh.updateAttribute(m_vertexRange, (m_transform.state.vertex().alpha), alphaOffset);
                m_updateMeshVisibility = false;
            }
        }

        m_dirty = false;
    }
}

void Label::resetState() {
    m_occludedLastFrame = false;
    m_occluded = false;
    m_updateMeshVisibility = true;
    m_dirty = true;
    m_proxy = false;
    enterState(State::wait_occ, 0.0);
}

bool Label::update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _zoomFract) {
    if (m_state == State::dead) {
        return false;
    }

    m_occluded = false;

    bool ruleSatisfied = updateScreenTransform(_mvp, _screenSize, true);

    // one of the label rules has not been satisfied
    if (!ruleSatisfied) {
        if ((m_state & State::wait_occ) != 0) {
            // go to dead state, this breaks determinism, but reduce potential
            // label set since a lot of discarded labels are discared for line
            // exceed (lots of tiny small lines on a curve for example), which
            // won't have their rule satisfied
            enterState(State::dead, 0.0);
            pushTransform();
        } else {
            enterState(State::sleep, 0.0);
            pushTransform();
        }
        return false;

    }

    // update the view-space bouding box
    updateBBoxes(_zoomFract);

    // checks whether the label is out of the viewport
    if (offViewport(_screenSize)) {
        enterState(State::out_of_screen, 0.0);
    }

    return true;
}

bool Label::evalState(const glm::vec2& _screenSize, float _dt) {

    bool animate = false;
    m_prevState = m_state;

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
                // enterState(State::sleep, 0.0);
                enterState(State::fading_out, m_transform.state.alpha);
                animate = true;
                break;
            }
            setAlpha(m_fade.update(_dt));
            animate = true;
            if (m_fade.isFinished() || (m_transform.state.alpha > 0.9f)) {
                enterState(State::visible, 1.0);
            }
            break;
        case State::fading_out:
            setAlpha(m_fade.update(_dt));
            animate = true;
            if (m_fade.isFinished() || (m_transform.state.alpha < 0.1f )) {
                enterState(State::sleep, 0.0);
            }
            break;
        case State::out_of_screen:
            if (!offViewport(_screenSize)) {
                enterState(State::wait_occ, 0.0);
            }
            break;
        case State::wait_occ:
            if (m_occluded) {
                enterState(State::dead, 0.0);

            } else {
                m_fade = FadeEffect(true, m_options.showTransition.ease,
                                    m_options.showTransition.time);
                enterState(State::fading_in, 0.0);
                animate = true;
            }
            break;
        case State::skip_transition:
            if (m_occluded) {
                enterState(State::dead, 0.0);

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
        case State::dead:
            break;
    }

    m_occludedLastFrame = m_occluded;
    m_occluded = false;

    return animate;
}

}
