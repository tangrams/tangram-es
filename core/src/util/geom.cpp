#include "geom.h"

#include <limits>
#include <cmath>
#include "glm/gtx/norm.hpp"

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

float signedArea(const std::vector<glm::vec3>& _polygon) {
    if (_polygon.empty()) { return 0; }
    float area = 0;
    auto prev = _polygon.back();
    for (const auto& curr : _polygon) {
        area += curr.x * prev.y - curr.y * prev.x;
        prev = curr;
    }
    return 0.5 * area;
}

float signedArea(const std::vector<glm::vec3>::const_iterator& _begin,
                 const std::vector<glm::vec3>::const_iterator& _end) {
    if (_begin == _end) { return 0; }
    float area = 0;
    glm::vec3 prev = *(_end-1);
    for (auto it = _begin; it != _end; ++it) {
        const auto& curr = *it;
        area += curr.x * prev.y - curr.y * prev.x;
        prev = curr;
    }
    return 0.5 * area;
}


glm::vec2 centroid(const std::vector<std::vector<glm::vec3>>& _polygon) {
    glm::vec2 centroid;
    int n = 0;

    for (auto& l : _polygon) {
        for (auto& p : l) {
            centroid.x += p.x;
            centroid.y += p.y;
            n++;
        }
    }

    if (n == 0) {
        return centroid;
    }

    centroid /= n;

    return centroid;
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

}
