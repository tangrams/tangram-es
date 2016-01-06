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
    void handleFlingGesture(float _posX, float _posY, float _velocityX, float _velocityY);
    void handlePinchGesture(float _posX, float _posY, float _scale, float _velocity);
    void handleRotateGesture(float _posX, float _posY, float _radians);
    void handleShoveGesture(float _distance);

    void update(float _dt);

    void setView(std::shared_ptr<View> _view) { m_view = _view; }

private:

    void setDeltas(float _zoom, glm::vec2 _translate);

    void onGesture();

    std::shared_ptr<View> m_view;

    bool m_gestureOccured = false;

    // fling deltas on zoom and translation
    glm::vec2 m_deltaTranslate;
    float m_deltaZoom = 0.f;

    /* Momentum config */

    // Damping factor for translation; reciprocal of the decay period in seconds
    const float m_dampingTranslate = 4.0f;

    // Damping factor for zoom; reciprocal of the decay period in seconds
    const float m_dampingZoom = 6.0f;

    // Minimum translation at which momentum should start
    const float m_thresholdStartTranslate = 8.f;

    // Minimum translation at which momentum should stop
    const float m_thresholdStopTranslate = 1.f;

    // Minimum zoom at which momentum should start
    const float m_thresholdStartZoom = 0.1f;

    // Minimum zoom at which momentum should stop
    const float m_thresholdStopZoom = 0.1f;

};

}
