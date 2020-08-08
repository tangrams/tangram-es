#include "util/geom.h"

#include "glm/gtx/norm.hpp"
#include <cmath>

namespace Tangram {

float mapRange01(float value, float inputMin, float inputMax) {
    if (inputMin == inputMax) {
        return value > inputMin ? 1 : 0;
    }
    return clamp01((value - inputMin) / (inputMax - inputMin));
}

glm::vec2 worldToScreenSpace(const glm::mat4& mvp, const glm::vec4& worldPosition, const glm::vec2& screenSize, bool& behindCamera) {
    glm::vec4 clip = worldToClipSpace(mvp, worldPosition);
    glm::vec3 ndc = clipSpaceToNdc(clip);
    glm::vec2 screenPosition = ndcToScreenSpace(ndc, screenSize);
    behindCamera = clipSpaceIsBehindCamera(clip);
    return screenPosition;
}

float pointSegmentDistanceSq(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) {
    // http://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
    glm::vec2 segment(b - a);
    float lengthSq = glm::length2(segment);
    if (lengthSq != 0) {
        float t = glm::dot(p - a, segment) / lengthSq;
        if (t > 1) {
            return glm::length2(p - b);
        } else if (t > 0) {
            return glm::length2(p - (a + segment * t));
        }
    }
    return glm::length2(p - a);
}

float pointSegmentDistance(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) {
    return sqrt(pointSegmentDistanceSq(p, a, b));
}

}
