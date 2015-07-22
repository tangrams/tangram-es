#include "inputHandler.h"

#include <cmath>
#include "glm/glm.hpp"
#include "platform.h"

InputHandler::InputHandler(std::shared_ptr<View> _view) : m_view(_view) {
    m_deltaZoom = 0.f;
    m_deltaTranslate = glm::vec2(0.f);
}

void InputHandler::update(float _dt) {

    bool hasMomentum = glm::length(m_deltaTranslate) > m_minDeltaLength || std::fabs(m_deltaZoom) > m_minDeltaZoomLength;

    if (!m_gestureOccured && hasMomentum) {

        // exponential decrease, m_expDecrease should be between [0..1]
        m_deltaTranslate *= m_expDecrease;
        // to give more control, make zoom decrease faster than translation
        m_deltaZoom *= m_expDecrease * m_expDecrease;

        m_view->translate(m_deltaTranslate.x, m_deltaTranslate.y);
        m_view->zoom(m_deltaZoom);

        requestRender();
    }

    m_gestureOccured = false;
}

void InputHandler::handleTapGesture(float _posX, float _posY) {

    float viewCenterX = 0.5f * m_view->getWidth();
    float viewCenterY = 0.5f * m_view->getHeight();

    m_view->screenToGroundPlane(viewCenterX, viewCenterY);
    m_view->screenToGroundPlane(_posX, _posY);

    m_view->translate((_posX - viewCenterX), (_posY - viewCenterY));

    onEndGesture();
}

void InputHandler::handleDoubleTapGesture(float _posX, float _posY) {

    handlePinchGesture(_posX, _posY, 2.f, 0.f);
}

void InputHandler::handlePanGesture(float _startX, float _startY, float _endX, float _endY) {

    // The platform can send the same values for the last registered pan
    if (std::abs(_startX - _endX) < FLT_EPSILON && std::abs(_startY - _endY) < FLT_EPSILON) {
        return;
    }

    m_view->screenToGroundPlane(_startX, _startY);
    m_view->screenToGroundPlane(_endX, _endY);

    float dx = _startX - _endX;
    float dy = _startY - _endY;

    setDeltas(0.f, glm::vec2(dx, dy));

    m_view->translate(dx, dy);

    onEndGesture();
}

void InputHandler::handlePinchGesture(float _posX, float _posY, float _scale, float _velocity) {

    float viewCenterX = 0.5f * m_view->getWidth();
    float viewCenterY = 0.5f * m_view->getHeight();

    m_view->screenToGroundPlane(viewCenterX, viewCenterY);
    m_view->screenToGroundPlane(_posX, _posY);

    float dx = (_posX - viewCenterX) * (1 - 1 / _scale);
    float dy = (_posY - viewCenterY) * (1 - 1 / _scale);

    m_view->translate(dx, dy);

    static float invLog2 = 1 / log(2);

    setDeltas(m_minZoomStart * _velocity, glm::vec2(0.f));

    m_view->zoom(log(_scale) * invLog2);

    onEndGesture();
}

void InputHandler::handleRotateGesture(float _posX, float _posY, float _radians) {

    m_view->screenToGroundPlane(_posX, _posY);
    m_view->orbit(_posX, _posY, _radians);

    onEndGesture();
}

void InputHandler::handleShoveGesture(float _distance) {

    m_view->pitch(_distance);

    onEndGesture();
}

void InputHandler::onEndGesture() {

    m_gestureOccured = true;

    // request a frame
    requestRender();
}

void InputHandler::setDeltas(float _zoom, glm::vec2 _translate) {

    // setup deltas for momentum on gesture
    m_deltaTranslate = _translate;
    m_deltaZoom = _zoom;

    if (glm::length(m_deltaTranslate) < m_minDeltaTranslate) {
        m_deltaTranslate = glm::vec2(0.f);
    }
}
