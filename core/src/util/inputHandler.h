#pragma once

#include "view/view.h"

#include <bitset>
#include <memory>

namespace Tangram {

class Platform;

class InputHandler {

public:
    InputHandler(std::shared_ptr<Platform> _platform, View& _view);

    void handleTapGesture(float _posX, float _posY);
    void handleDoubleTapGesture(float _posX, float _posY);
    void handlePanGesture(float _startX, float _startY, float _endX, float _endY);
    void handleFlingGesture(float _posX, float _posY, float _velocityX, float _velocityY);
    void handlePinchGesture(float _posX, float _posY, float _scale, float _velocity);
    void handleRotateGesture(float _posX, float _posY, float _radians);
    void handleShoveGesture(float _distance);

    void update(float _dt);

    void cancelFling();

    void setView(View& _view) { m_view = _view; }

private:

    void setVelocity(float _zoom, glm::vec2 _pan);

    void onGesture();

    std::shared_ptr<Platform> m_platform;

    View& m_view;

    // fling deltas on zoom and translation
    glm::vec2 m_velocityPan;
    float m_velocityZoom = 0.f;

};

}
