#include "label.h"

Label::Label(Transform _transform, std::string _text, std::shared_ptr<TextBuffer> _buffer, Type _type) :
    m_type(_type),
    m_transform(_transform),
    m_text(_text),
    m_buffer(_buffer) {

    m_id = m_buffer->genTextID();

    m_currentState = m_type == Type::DEBUG ? State::VISIBLE : State::WAIT_OCC;
    m_occludedLastFrame = false;
    m_occlusionSolved = false;
}

Label::~Label() {}

void Label::rasterize() {
    m_buffer->rasterize(m_text, m_id);

    glm::vec4 bbox = m_buffer->getBBox(m_id);

    m_dim.x = std::abs(bbox.z - bbox.x);
    m_dim.y = std::abs(bbox.w - bbox.y);
}

void Label::updateBBoxes() {
    glm::vec2 t = glm::vec2(cos(m_transform.m_rotation), sin(m_transform.m_rotation));
    glm::vec2 tperp = glm::vec2(-t.y, t.x);
    glm::vec2 obbCenter;

    obbCenter = m_transform.m_screenPosition + t * (m_dim.x / 2) - tperp * (m_dim.y / 8);

    m_obb = isect2d::OBB(obbCenter.x, obbCenter.y, m_transform.m_rotation, m_dim.x, m_dim.y);
    m_aabb = m_obb.getExtent();
}

void Label::pushTransform() {
    m_buffer->transformID(m_id, m_transform.m_screenPosition.x, m_transform.m_screenPosition.y, m_transform.m_rotation, m_transform.m_alpha);
}

bool Label::updateScreenTransform(const glm::mat4& _mvp, const glm::vec2& _screenSize) {

    glm::vec2 screenPosition;
    float rot = 0;

    switch (m_type) {
        case Type::DEBUG:
        case Type::POINT:
        {
            glm::vec4 v1 = worldToClipSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0));

            if (v1.w <= 0) {
                return false;
            }

            m_depth = v1.w;

            screenPosition = clipToScreenSpace(v1, _screenSize);

            // center on half the width
            screenPosition.x -= m_dim.x / 2;

            break;
        }
        case Type::LINE:
        {
            // project label position from mercator world space to clip coordinates
            glm::vec4 v1 = worldToClipSpace(_mvp, glm::vec4(m_transform.m_modelPosition1, 0.0, 1.0));
            glm::vec4 v2 = worldToClipSpace(_mvp, glm::vec4(m_transform.m_modelPosition2, 0.0, 1.0));

            // check whether the label is behind the camera using the perspective division factor
            if (v1.w <= 0 || v2.w <= 0) {
                return false;
            }

            m_depth = (v1.w + v2.w) / 2.f;

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

            float exceedHeuristic = 40; // default heuristic : 40%

            if (m_dim.x > length) {
                float exceed = (1 - (length / m_dim.x)) * 100;
                if (exceed < exceedHeuristic) {
                    return false;
                }
            }

            screenPosition = (p1 + p2) / 2.0f;

            // translate by half the width
            screenPosition += t * (m_dim.x / 2);

            break;
        }

        default:
            break;
    }

    m_transform.m_screenPosition = screenPosition;
    m_transform.m_rotation = rot;

    return true;
}

void Label::update(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt) {

    updateState(_mvp, _screenSize, _dt);
}

bool Label::offViewport(const glm::vec2& _screenSize) {
    const glm::vec2& screenPosition = m_transform.m_screenPosition;

    bool outOfScreen = screenPosition.x > _screenSize.x || screenPosition.x < 0;
    outOfScreen = outOfScreen || screenPosition.y > _screenSize.y || screenPosition.y < 0;

    return outOfScreen;
}

void Label::setOcclusion(bool _occlusion) {
    if (!canOcclude()) {
        return;
    }

    m_occludedLastFrame = _occlusion;
}

bool Label::canOcclude() {
    int occludeFlags = (State::VISIBLE | State::WAIT_OCC);
    return (occludeFlags & m_currentState) && !(m_type == Type::DEBUG);
}

void Label::occlusionSolved() {
    m_occlusionSolved = true;
}

void Label::enterState(State _state) {
    m_currentState = _state;
    m_transform.m_alpha = CLAMP(m_transform.m_alpha, 0.0, 1.0);
}

void Label::updateState(const glm::mat4& _mvp, const glm::vec2& _screenSize, float _dt) {
    bool occludedLasteFrame = m_occludedLastFrame;
    m_occludedLastFrame = false;

    bool ruleSatisfied = updateScreenTransform(_mvp, _screenSize);

    if (!ruleSatisfied) { // label rules not satisfied
        m_transform.m_alpha = 0.0;
        return;
    }

    updateBBoxes();

    if (m_currentState != State::SLEEP && offViewport(_screenSize)) {
        enterState(State::OUT_OF_SCREEN);
    }

    switch (m_currentState) {
        case State::VISIBLE:
            m_transform.m_alpha = 1.0;

            if (m_depth > 100.0f) {
                enterState(State::SLEEP);
            } else if (occludedLasteFrame) {
                enterState(State::FADING_OUT);
            }
            break;

        case State::FADING_IN:
            m_transform.m_alpha += 0.03;

            if (m_transform.m_alpha >= 1.0) {
                enterState(State::VISIBLE);
            }
            break;

        case State::FADING_OUT:
            m_transform.m_alpha -= 0.03;

            if (m_transform.m_alpha <= 0.0) {
                enterState(State::SLEEP);
            }
            break;

        case State::OUT_OF_SCREEN:
            m_transform.m_alpha = 0.0;

            if (!offViewport(_screenSize)) {
                enterState(State::VISIBLE);
            }
            break;

        case State::SLEEP:
            m_transform.m_alpha = 0.0;
            break;

        case State::WAIT_OCC:
            m_transform.m_alpha = 0.0;

            if (!occludedLasteFrame && m_occlusionSolved) {
                enterState(State::VISIBLE);
            } else if (occludedLasteFrame) {
                enterState(State::STATE_N);
            }
            break;

        default:
            break;
    }

    m_occlusionSolved = false;
}

