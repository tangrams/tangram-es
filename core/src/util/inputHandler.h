#pragma once

#include "view/view.h"
#include <memory>

class InputHandler {
    
public:
    InputHandler(std::shared_ptr<View> _view);
    
    void handleTapGesture(float _posX, float _posY);
    void handleDoubleTapGesture(float _posX, float _posY);
    void handlePanGesture(float _startX, float _startY, float _endX, float _endY);
    void handlePinchGesture(float _posX, float _posY, float _scale);
    void handleRotateGesture(float _posX, float _posY, float _radians);
    void handleShoveGesture(float _distance);
    
    void update(float _dt);
    
private:
    std::shared_ptr<View> m_view;
        
    bool m_hasPan;
    glm::vec2 m_panDelta;
    const float m_minPanDelta = 5.f;
    const float m_decreasePan = 0.95f;

};