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

glm::vec4 worldToClipSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition) {
    return _mvp * _worldPosition;
}

glm::vec2 clipToScreenSpace(const glm::vec4& _clipCoords, const glm::vec2& _screenSize) {
    glm::vec2 halfScreen = glm::vec2(_screenSize * 0.5f);

    // from normalized device coordinates to screen space coordinate system
    // top-left screen axis, y pointing down

    glm::vec2 screenPos;
    screenPos.x = (_clipCoords.x / _clipCoords.w) + 1;
    screenPos.y = 1 - (_clipCoords.y / _clipCoords.w);

    return screenPos * halfScreen;
}

glm::vec2 worldToScreenSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition, const glm::vec2& _screenSize) {
    return clipToScreenSpace(worldToClipSpace(_mvp, _worldPosition), _screenSize);
}

glm::vec2 worldToScreenSpace(const glm::mat4& _mvp, const glm::vec4& _worldPosition, const glm::vec2& _screenSize, bool& _clipped) {

    glm::vec4 clipCoords = worldToClipSpace(_mvp, _worldPosition);

    if (clipCoords.w <= 0.0f) {
        _clipped = true;
        //return {};
    }

    return clipToScreenSpace(clipCoords, _screenSize);
}

glm::vec2 worldToScreenSpaceDirection(const glm::mat4& mvp, const glm::vec4& worldDirection, const glm::vec2& screenSize) {

    glm::vec4 clipCoords = worldToClipSpace(mvp, worldDirection);

    glm::vec2 screenScale = screenSize * 0.5f;

    glm::vec2 clipDirection{ clipCoords.x, clipCoords.y };

    return clipDirection * screenScale;
}

glm::vec2 worldToScreenSpaceClamped(const glm::mat4& mvp, const glm::vec4& worldPosition, const glm::vec4& worldScreenOrigin, const glm::vec2& screenSize, float f, bool& clamped) {
#if 1
    glm::vec4 clip = worldToClipSpace(mvp, worldPosition);
    glm::vec3 ndc = glm::vec3(clip) / clip.w;

    if (abs(ndc.x) <= 1 && abs(ndc.y) <= 1 && abs(ndc.z) <= 1) {
        clamped = false;
    } else {
        clamped = true;
        // Point is off-screen, so get the direction to it and determine the point on the screen edge in that direction.
        glm::vec4 worldDirection = worldPosition - worldScreenOrigin;
        worldDirection.w = 0;

        glm::vec4 clipDirection = worldToClipSpace(mvp, worldDirection);

        ndc = glm::vec3(clipDirection) / glm::max(abs(clipDirection.x), abs(clipDirection.y));
    }

    glm::vec2 screenPosition = glm::vec2(1 + ndc.x, 1 - ndc.y) * screenSize * 0.5f;
    return screenPosition;
#else
    glm::vec4 clipCoords = worldToClipSpace(mvp, worldPosition);
    glm::vec3 ndc = glm::vec3(clipCoords) / clipCoords.w;

    if (ndc.z >= -1.0f && ndc.z <= 1.0f) {
        // Within ndc clipping range on z, simply clamp
        ndc.x = glm::clamp(ndc.x, -1.0f, 1.0f);
        ndc.y = glm::clamp(ndc.y, -1.0f, 1.0f);
    } else if (ndc.z > f) {
        // Behind frustum, classify in 4 screen sections, inverted
        glm::vec2 rightEdge[2]  = {glm::vec2(-1.0f,-1.0f), glm::vec2(-1.0f, 1.0f)};
        glm::vec2 topEdge[2]    = {glm::vec2(-1.0f,-1.0f), glm::vec2( 1.0f,-1.0f)};
        glm::vec2 leftEdge[2]   = {glm::vec2( 1.0f,-1.0f), glm::vec2( 1.0f, 1.0f)};
        glm::vec2 bottomEdge[2] = {glm::vec2(-1.0f, 1.0f), glm::vec2( 1.0f, 1.0f)};

        float leftEdgeDistance2   = sqSegmentDistance(glm::vec2(ndc), leftEdge[0], leftEdge[1]);
        float topEdgeDistance2    = sqSegmentDistance(glm::vec2(ndc), topEdge[0], topEdge[1]);
        float rightEdgeDistance2  = sqSegmentDistance(glm::vec2(ndc), rightEdge[0], rightEdge[1]);
        float bottomEdgeDistance2 = sqSegmentDistance(glm::vec2(ndc), bottomEdge[0], bottomEdge[1]);

        float minDistance2 = std::min<float>(
                std::min<float>(leftEdgeDistance2, topEdgeDistance2),
                std::min<float>(rightEdgeDistance2, bottomEdgeDistance2));

        // Select and fall through
        if (minDistance2 == leftEdgeDistance2) {
            ndc.x = -1.0f;
            ndc.y = -glm::clamp(ndc.y, -1.0f, 1.0f);
        } else if (minDistance2 == rightEdgeDistance2) {
            ndc.x = 1.0f;
            ndc.y = -glm::clamp(ndc.y, -1.0f, 1.0f);
        } else if (minDistance2 == topEdgeDistance2) {
            ndc.y = 1.0f;
            ndc.x = -glm::clamp(ndc.x, -1.0f, 1.0f);
        } else if (minDistance2 == bottomEdgeDistance2) {
            ndc.y = -1.0f;
            ndc.x = -glm::clamp(ndc.x, -1.0f, 1.0f);
        }
    } else if (ndc.z > 1.0f) {
        // Beyond far plane
    } else if (ndc.z < -1.0f) {
        // Between camera and near plane
        assert(false);
    }

    glm::vec2 screenPos;
    screenPos.x = ndc.x + 1.0f;
    screenPos.y = 1.0f - ndc.y;

    return screenPos * glm::vec2(screenSize * 0.5f);
#endif
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
