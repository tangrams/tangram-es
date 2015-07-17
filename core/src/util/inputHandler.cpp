#include "inputHandler.h"

#include <cmath>
#include "glm/glm.hpp"
#include "platform.h"

InputHandler::InputHandler(std::shared_ptr<View> _view) : m_view(_view) {}

void InputHandler::update(float _dt) {
    
    if (!m_hasPan && glm::length(m_panDelta) > 0.f) {
        // exponential decrease, m_decreasePan should be between [0..1]
        m_panDelta *= m_decreasePan;
        m_view->translate(m_panDelta.x, m_panDelta.y);
        
        requestRender();
    }
    
    m_hasPan = false;
}

void InputHandler::handleTapGesture(float _posX, float _posY) {
    
    // break pan momentum
    m_panDelta = glm::vec2(0.f);
    
    float viewCenterX = 0.5f * m_view->getWidth();
    float viewCenterY = 0.5f * m_view->getHeight();
    
    m_view->screenToGroundPlane(viewCenterX, viewCenterY);
    m_view->screenToGroundPlane(_posX, _posY);
    
    m_view->translate((_posX - viewCenterX), (_posY - viewCenterY));
    
    requestRender();
}

void InputHandler::handleDoubleTapGesture(float _posX, float _posY) {
    
    handlePinchGesture(_posX, _posY, 2.f);
}

void InputHandler::handlePanGesture(float _startX, float _startY, float _endX, float _endY) {
    
    // it is possible that the platform is sending the same values for the last pan
    if (std::abs(_startX - _endX) < FLT_EPSILON && std::abs(_startY - _endY) < FLT_EPSILON) {
        return;
    }
    
    m_view->screenToGroundPlane(_startX, _startY);
    m_view->screenToGroundPlane(_endX, _endY);
    
    float dx = _startX - _endX;
    float dy = _startY - _endY;
    
    m_panDelta = glm::vec2(dx, dy);
    
    if (glm::length(m_panDelta) < m_minPanDelta) {
        m_panDelta = glm::vec2(0.f);
    }
    
    m_view->translate(dx, dy);
    
    m_hasPan = true;
    
    requestRender();
}

void InputHandler::handlePinchGesture(float _posX, float _posY, float _scale) {
    
    float viewCenterX = 0.5f * m_view->getWidth();
    float viewCenterY = 0.5f * m_view->getHeight();
    
    m_view->screenToGroundPlane(viewCenterX, viewCenterY);
    m_view->screenToGroundPlane(_posX, _posY);
    
    m_view->translate((_posX - viewCenterX)*(1-1/_scale), (_posY - viewCenterY)*(1-1/_scale));
    
    static float invLog2 = 1 / log(2);
    m_view->zoom(log(_scale) * invLog2);
    
    requestRender();
}

void InputHandler::handleRotateGesture(float _posX, float _posY, float _radians) {
    
    m_view->screenToGroundPlane(_posX, _posY);
    m_view->orbit(_posX, _posY, _radians);
    
    requestRender();
}

void InputHandler::handleShoveGesture(float _distance) {
    
    m_view->pitch(_distance);
    
    requestRender();
}
