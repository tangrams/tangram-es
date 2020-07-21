#include "util/geom.h"

#include "glm/gtx/norm.hpp"
#include <cmath>
#include <limits>

namespace Tangram {

float mapValue(const float& _value, const float& _inputMin, const float& _inputMax, const float& _outputMin,
               const float& _outputMax, bool _clamp) {
    if (fabs(_inputMin - _inputMax) < std::numeric_limits<float>::epsilon()) { return _outputMin; } else {
        float outVal = ((_value - _inputMin) / (_inputMax - _inputMin) * (_outputMax - _outputMin) + _outputMin);

        if (_clamp) {
            if (_outputMax < _outputMin) {
                if (outVal < _outputMax) {
                    outVal = _outputMax;
                } else if (outVal > _outputMin) {
                    outVal = _outputMin;
                }
            } else {
                if (outVal > _outputMax) {
                    outVal = _outputMax;
                } else if (outVal < _outputMin) {
                    outVal = _outputMin;
                }
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

float sqPointSegmentDistance(const glm::vec2& _p, const glm::vec2& _a, const glm::vec2& _b) {

    float dx = _b.x - _a.x;
    float dy = _b.y - _a.y;

    float x = _a.x;
    float y = _a.y;

    float d = dx * dx + dy * dy;

    if (d != 0) {
        // project point onto segment
        float t = ((_p.x - _a.x) * dx + (_p.y - _a.y) * dy) / d;
        if (t > 1) {
            x = _b.x;
            y = _b.y;
        } else if (t > 0) {
            x += dx * t;
            y += dy * t;
        }
    }

    dx = _p.x - x;
    dy = _p.y - y;

    return dx * dx + dy * dy;
}

float pointSegmentDistance(const glm::vec2& _p, const glm::vec2& _a, const glm::vec2& _b) {
    return sqrt(sqPointSegmentDistance(_p, _a, _b));
}

glm::vec2 worldToScreenSpace(const glm::mat4& mvp, const glm::vec4& worldPosition, const glm::vec2& screenSize, bool& behindCamera) {
    glm::vec4 clip = worldToClipSpace(mvp, worldPosition);
    glm::vec3 ndc = clipSpaceToNdc(clip);
    glm::vec2 screenPosition = ndcToScreenSpace(ndc, screenSize);
    behindCamera = clipSpaceIsBehindCamera(clip);
    return screenPosition;
}

// square distance from a point <_p> to a segment <_p1,_p2>
// http://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
//
float sqSegmentDistance(const glm::vec2& _p, const glm::vec2& _p1, const glm::vec2& _p2) {
    glm::vec2 d(_p2 - _p1);
    float lengthSq = glm::length2(d);

    if (lengthSq != 0) {

        float t = glm::dot(_p - _p1, d) / lengthSq;

        if (t > 1) {
            return glm::length2(_p - _p2);
        } else if (t > 0) {
            return glm::length2(_p - (_p1 + d * t));
        }
    }
    return glm::length2(_p - _p1);
}

bool isPowerOfTwo(int _value) {
    return (_value & (_value - 1)) == 0;
}

}
