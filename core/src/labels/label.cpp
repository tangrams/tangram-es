#include "label.h"

#include "util/geom.h"
#include "labels/labelMesh.h"

namespace Tangram {

Label::Label(Label::Transform _transform, glm::vec2 _size, Type _type, LabelMesh& _mesh, Range _vertexRange, Options _options) :
    m_type(_type),
    m_options(_options),
    m_transform(_transform),
    m_dim(_size),
    m_mesh(_mesh),
    m_vertexRange(_vertexRange) {

    m_transform.state.alpha = m_type == Type::debug ? 1.0 : 0.0;
    m_currentState = m_type == Type::debug ? State::visible : State::wait_occ;
    m_occludedLastFrame = false;
    m_occlusionSolved = false;
    m_updateMeshVisibility = true;
    m_dirty = true;
}

Label::~Label() {}

bool Label::updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize) {

    glm::vec2 screenPosition;
    float rot = 0;

    switch (m_type) {
        case Type::debug:
        case Type::point:
        {
            glm::vec4 v1 = worldToClipSpace(_mvp, glm::vec4(m_transform.modelPosition1, 0.0, 1.0));

            if (v1.w <= 0) {
                return false;
            }

            screenPosition = clipToScreenSpace(v1, _screenSize);

            // center on half the width
            screenPosition.x -= m_dim.x * 0.5f;

            break;
        }
        case Type::line:
        {
            // project label position from mercator world space to clip coordinates
            glm::vec4 v1 = worldToClipSpace(_mvp, glm::vec4(m_transform.modelPosition1, 0.0, 1.0));
            glm::vec4 v2 = worldToClipSpace(_mvp, glm::vec4(m_transform.modelPosition2, 0.0, 1.0));

            // check whether the label is behind the camera using the perspective division factor
            if (v1.w <= 0 || v2.w <= 0) {
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

            glm::vec2 p1p2 = p2 - p1;
            glm::vec2 t = glm::normalize(-p1p2);

            float length = glm::length(p1p2);

            float exceedHeuristic = 80; // default heuristic : 80%

            if (m_dim.x > length) {
                float exceed = (1 - (length / m_dim.x)) * 100;
                if (exceed > exceedHeuristic) {
                    return false;
                }
            }

            screenPosition = (p1 + p2) * 0.5f + t * m_dim.x * 0.5f;

            break;
        }
    }

    setScreenPosition(screenPosition);
    setRotation(rot);

    return true;
}

bool Label::update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt) {
    bool animate =  updateState(_mvp, _screenSize, _dt);
    m_occlusionSolved = false;

    return animate;
}

bool Label::offViewport(const glm::vec2& _screenSize) {
    const isect2d::Vec2* quad = m_obb.getQuad();

    for (int i = 0; i < 4; ++i) {
        const auto& p = quad[i];
        if (p.x < _screenSize.x && p.y < _screenSize.y && p.x > 0 && p.y > 0) {
            return false;
        }
    }

    return true;
}

void Label::setOcclusion(bool _occlusion) {
    if (!canOcclude()) {
        return;
    }

    m_occludedLastFrame = _occlusion;
}

bool Label::canOcclude() {
    int occludeFlags = (State::visible | State::wait_occ | State::fading_in);
    return (occludeFlags & m_currentState) && !(m_type == Type::debug);
}

bool Label::visibleState() const {
    int visibleFlags = (State::visible | State::fading_in | State::fading_out);
    return (visibleFlags & m_currentState);
}

void Label::occlusionSolved() {
    m_occlusionSolved = true;
}

void Label::enterState(State _state, float _alpha) {
    m_currentState = _state;
    setAlpha(_alpha);
}

void Label::setAlpha(float _alpha) {
    float alpha = CLAMP(_alpha, 0.0, 1.0);
    if (m_transform.state.alpha != alpha) {
        m_transform.state.alpha = alpha;
        m_dirty = true;

        if (alpha == 0.f) {
            m_updateMeshVisibility = true;
        }
    }
}

void Label::setScreenPosition(const glm::vec2& _screenPosition) {
    if (_screenPosition != m_transform.state.screenPos) {
        m_transform.state.screenPos = _screenPosition + m_options.offset;
        m_dirty = true;
    }
}

void Label::setRotation(float _rotation) {
    m_transform.state.rotation = _rotation;
    m_dirty = true;
}

void Label::pushTransform() {

    // update the buffer on valid states
    if (m_dirty) {
        static size_t attribOffset = offsetof(Label::Vertex, state);
        static size_t alphaOffset = offsetof(Label::Vertex::State, alpha) + attribOffset;

        if (visibleState()) {
            // update the complete state on the mesh
            m_mesh.updateAttribute(m_vertexRange, m_transform.state, attribOffset);
        } else {

            // for any non-visible states, we don't need to overhead the gpu with updates on the
            // alpha attribute, but simply do it once until the label goes back in a visible state
            if (m_updateMeshVisibility) {
                m_mesh.updateAttribute(m_vertexRange, m_transform.state.alpha, alphaOffset);
                m_updateMeshVisibility = false;
            }
        }

        m_dirty = false;
    }
}

bool Label::updateState(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt) {
    if (m_currentState == State::dead) {
        // no-op state for now, when label-collision has less complexity, this state
        // would lead to FADE_IN state if no collision occured
        return false;
    }

    bool occludedLastFrame = m_occludedLastFrame;
    m_occludedLastFrame = false;

    bool ruleSatisfied = updateScreenTransform(_mvp, _screenSize);

    if (!ruleSatisfied) { // one of the label rules not satisfied
        enterState(State::sleep, 0.0);
        return false;
    }

    // update the view-space bouding box
    updateBBoxes();

    // checks whether the label is out of the viewport
    if (offViewport(_screenSize)) {
        enterState(State::out_of_screen, 0.0);
    }

    bool animate = false;

    switch (m_currentState) {
        case State::visible:
            if (occludedLastFrame) {
                m_fade = FadeEffect(false, FadeEffect::Interpolation::sine, 0.2);
                enterState(State::fading_out, 1.0);
            }
            break;
        case State::fading_in:
            if (occludedLastFrame) {
                enterState(State::sleep, 0.0);
                break;
            }
            setAlpha(m_fade.update(_dt));
            animate = true;
            if (m_fade.isFinished()) {
                enterState(State::visible, 1.0);
            }
            break;
        case State::fading_out:
            setAlpha(m_fade.update(_dt));
            animate = true;
            if (m_fade.isFinished()) {
                enterState(State::sleep, 0.0);
            }
            break;
        case State::out_of_screen:
            if (!offViewport(_screenSize)) {
                enterState(State::wait_occ, 0.0);
            }
            break;
        case State::wait_occ:
            if (m_occlusionSolved) {
                if (occludedLastFrame) {
                    enterState(State::dead, 0.0); // dead
                }  else {
                    m_fade = FadeEffect(true, FadeEffect::Interpolation::pow, 0.2);
                    enterState(State::fading_in, 0.0);
                }
            }
            break;
        case State::sleep:
            if (!occludedLastFrame) {
                m_fade = FadeEffect(true, FadeEffect::Interpolation::pow, 0.2);
                enterState(State::fading_in, 0.0);
            }
            break;
        case State::dead:;
    }

    return animate;
}

}
