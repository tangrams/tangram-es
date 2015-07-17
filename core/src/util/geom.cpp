#include "geom.h"

#include <limits>
#include <cmath>

float mapValue(const float& _value, const float& _inputMin, const float& _inputMax, const float& _outputMin,
               const float& _outputMax, bool _clamp) {
    if (fabs(_inputMin - _inputMax) < std::numeric_limits<float>::epsilon()) { return _outputMin; } else {
        float outVal = ((_value - _inputMin) / (_inputMax - _inputMin) * (_outputMax - _outputMin) + _outputMin);

        if (_clamp) {
            if (_outputMax < _outputMin) {
                if (outVal < _outputMax)
                    outVal = _outputMax;
                else if (outVal > _outputMin)
                    outVal = _outputMin;
            } else {
                if (outVal > _outputMax)
                    outVal = _outputMax;
                else if (outVal < _outputMin)
                    outVal = _outputMin;
            }
        }
        return outVal;
    }
}

float angleBetweenPoints(const glm::vec2& _p1, const glm::vec2& _p2) {
    glm::vec2 p1p2 = _p2 - _p1;
    p1p2 = glm::normalize(p1p2);
    return (float)atan2(p1p2.x, -p1p2.y);
}

glm::vec4 worldToClipSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition) {
    return _mvp * _worldPosition;
}

glm::vec2 clipToScreenSpace(const glm::vec4& _clipCoords, const glm::vec2& _screenSize) {
    glm::vec2 halfScreen = glm::vec2(_screenSize * 0.5f);

    glm::vec4 ndc = _clipCoords / _clipCoords.w;

    // from normalized device coordinates to screen space coordinate system
    // top-left screen axis, y pointing down

    return glm::vec2((ndc.x + 1) * halfScreen.x, (1 - ndc.y) * halfScreen.y);
}

glm::vec2 worldToScreenSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition, const glm::vec2& _screenSize) {
    return clipToScreenSpace(worldToClipSpace(_mvp, _worldPosition), _screenSize);
}
