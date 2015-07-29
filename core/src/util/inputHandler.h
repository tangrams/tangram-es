#pragma once

#include "view/view.h"
#include <memory>
#include <bitset>

namespace Tangram {

class InputHandler {

public:
    InputHandler(std::shared_ptr<View> _view);

    void handleTapGesture(float _posX, float _posY);
    void handleDoubleTapGesture(float _posX, float _posY);
    void handlePanGesture(float _startX, float _startY, float _endX, float _endY);
    void handlePinchGesture(float _posX, float _posY, float _scale, float _velocity);
    void handleRotateGesture(float _posX, float _posY, float _radians);
    void handleShoveGesture(float _distance);

    void handlePinchGestureEnd();
    void handleRotateGestureEnd();

    void update(float _dt);

private:
    
    enum GestureFlags {
        tap = 0,
        double_tap,
        pan,
        pinch,
        rotate,
        shove
    };

    void setDeltas(float _zoom, glm::vec2 _translate);
    void clearDeltas();

    void onEndGesture();

    std::shared_ptr<View> m_view;

    bool m_gestureOccured = false;
    bool m_momentumHandled = false;

    // fling deltas on zoom and translation
    glm::vec2 m_deltaTranslate;
    float m_deltaZoom;
    
    std::bitset<8> m_gestures = 0;

    glm::vec2 mPrevFocus = glm::vec2(0.0f, 0.0f);

    /* Momentum config */

    const float m_expDecrease = 0.95f;

    // the minimum translation at which a momentum should start
    const float m_minDeltaTranslate = 8.f;
    // the minimum translation at which momentum should stop
    const float m_minDeltaLength = 0.1f;
    // the minimum zoom at which momentum should stop
    const float m_minDeltaZoomLength = 1e-4f;
    // the minimum zoom value at which momentum will start
    const float m_minZoomStart = 0.05f;

};

}
