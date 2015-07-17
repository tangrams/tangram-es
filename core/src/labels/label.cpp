#include "label.h"

#include "util/geom.h"

bool Label::s_needUpdate = false;

Label::Label(Label::Transform _transform, std::string _text, fsuint _id, Type _type) :
    m_type(_type),
    m_transform(_transform),
    m_text(_text), m_id(_id) {

    m_transform.m_alpha = m_type == Type::debug ? 1.0 : 0.0;
    m_currentState = m_type == Type::debug ? State::visible : State::wait_occ;
    m_occludedLastFrame = false;
    m_occlusionSolved = false;
}

Label::~Label() {}

bool Label::rasterize(std::shared_ptr<TextBuffer>& _buffer) {
    bool res = _buffer->rasterize(m_text, m_id);

    if (!res) {
        return false;
    }

    glm::vec4 bbox = _buffer->getBBox(m_id);

    m_dim.x = std::abs(bbox.z - bbox.x);
    m_dim.y = std::abs(bbox.w - bbox.y);

    return true;
}

void Label::updateBBoxes() {
    glm::vec2 t = glm::vec2(cos(m_transform.m_rotation), sin(m_transform.m_rotation));
    glm::vec2 tperp = glm::vec2(-t.y, t.x);
    glm::vec2 obbCenter;

    obbCenter = m_transform.m_screenPosition + t * m_dim.x * 0.5f - tperp * (m_dim.y / 8);

    m_obb = isect2d::OBB(obbCenter.x, obbCenter.y, m_transform.m_rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}

void Label::pushTransform(std::shared_ptr<TextBuffer>& _buffer) {
    if (m_dirty) {
        _buffer->transformID(m_id, m_transform.m_screenPosition.x, m_transform.m_screenPosition.y, m_transform.m_rotation, m_transform.m_alpha);
        m_dirty = false;
    }
}

bool Label::updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize) {

    glm::vec2 screenPosition;
    float rot = 0;

    switch (m_type) {
        case Type::debug:
        case Type::point:
        {
            glm::vec4 v1 = worldToClipSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0));

            if (v1.w <= 0) {
                return false;
            }

            screenPosition = clipToScreenSpace(v1, _screenSize);

            // center on half the width
            screenPosition.x -= m_dim.x / 2;

            break;
        }
        case Type::line:
        {
            // project label position from mercator world space to clip coordinates
            glm::vec4 v1 = worldToClipSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0));
            glm::vec4 v2 = worldToClipSpace(_mvp, glm::vec4(m_transform.m_modelPosition2, 0.0, 1.0));

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
            glm::vec2 tperp = glm::vec2(-t.y, t.x);

            float length = glm::length(p1p2);

            float exceedHeuristic = 30; // default heuristic : 30%

            if (m_dim.x > length) {
                float exceed = (1 - (length / m_dim.x)) * 100;
                if (exceed > exceedHeuristic) {
                    return false;
                }
            }

            screenPosition = (p1 + p2) * 0.5f + t * m_dim.x * 0.5f - tperp * (m_dim.y / 6);

            break;
        }
    }

    setScreenPosition(screenPosition);
    setRotation(rot);

    return true;
}

void Label::update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt) {
    updateState(_mvp, _screenSize, _dt);

    m_occlusionSolved = false;
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

void Label::occlusionSolved() {
    m_occlusionSolved = true;
}

void Label::enterState(State _state, float _alpha) {
    m_currentState = _state;
    setAlpha(_alpha);
}

void Label::setAlpha(float _alpha) {
    m_transform.m_alpha = CLAMP(_alpha, 0.0, 1.0);
    m_dirty = true;
}

void Label::setScreenPosition(const glm::vec2& _screenPosition) {
    if (_screenPosition != m_transform.m_screenPosition) {
        m_transform.m_screenPosition = _screenPosition;
        m_dirty = true;
    }
}

void Label::setRotation(float _rotation) {
    m_transform.m_rotation = _rotation;
    m_dirty = true;
}

void Label::updateState(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt) {
    if (m_currentState == State::sleep) {
        // no-op state for now, when label-collision has less complexity, this state
        // would lead to FADE_IN state if no collision occured
        return;
    }

    bool occludedLastFrame = m_occludedLastFrame;
    m_occludedLastFrame = false;

    bool ruleSatisfied = updateScreenTransform(_mvp, _screenSize);

    if (!ruleSatisfied) { // one of the label rules not satisfied
        enterState(State::sleep, 0.0);
        return;
    }

    // update the view-space bouding box
    updateBBoxes();

    // checks whether the label is out of the viewport
    if (offViewport(_screenSize)) {
        enterState(State::out_of_screen, 0.0);
    }

    switch (m_currentState) {
        case State::visible:
            if (occludedLastFrame) {
                m_fade = FadeEffect(false, FadeEffect::Interpolation::sine, 1.0);
                enterState(State::fading_out, 1.0);
            }
            break;
        case State::fading_in:
            if (occludedLastFrame) {
                enterState(State::sleep, 0.0);
                break;
            }
            setAlpha(m_fade.update(_dt));
            s_needUpdate = true;
            if (m_fade.isFinished())
                enterState(State::visible, 1.0);
            break;
        case State::fading_out:
            setAlpha(m_fade.update(_dt));
            s_needUpdate = true;
            if (m_fade.isFinished())
                enterState(State::sleep, 0.0);
            break;
        case State::out_of_screen:
            if (!offViewport(_screenSize))
                enterState(State::wait_occ, 0.0);
            break;
        case State::wait_occ:
            if (!occludedLastFrame && m_occlusionSolved) {
                m_fade = FadeEffect(true, FadeEffect::Interpolation::pow, 0.2);
                enterState(State::fading_in, 0.0);
            } else if (occludedLastFrame && m_occlusionSolved) {
                enterState(State::sleep, 0.0);
            }
            break;
        case State::sleep:;
            // dead state
    }
}

