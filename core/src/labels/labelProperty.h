#pragma once

#include "util/util.h"

#include "aabb.h"
#include "glm/vec2.hpp"
#include <string>
#include <array>

namespace Tangram {

namespace LabelProperty {

enum Placement : uint8_t {
    vertex = 0,
    midpoint,
    spaced,
    centroid,
};

enum Anchor : uint8_t {
    center = 0,
    top,
    bottom,
    left,
    right,
    top_left,
    top_right,
    bottom_left,
    bottom_right,
};

constexpr int max_anchors = 9;

struct Anchors {
    std::array<LabelProperty::Anchor, LabelProperty::max_anchors> anchor;
    int count = 0;

    LabelProperty::Anchor operator[](size_t _pos) const {
        return anchor[_pos];
    }

    bool operator==(const Anchors& _other) const {
        return anchor == _other.anchor && count == _other.count;
    }

    isect2d::AABB<glm::vec2> extents(glm::vec2 size) {

        glm::vec2 min{0};
        glm::vec2 max{0};

        for (int i = 0; i < count; i++) {
            switch(anchor[i]) {
            case center:
                min.x = std::min(min.x, -0.5f);
                min.y = std::min(min.y, -0.5f);
                max.x = std::max(max.x, 0.5f);
                max.y = std::max(max.y, 0.5f);
                break;
            case top:
                min.x = std::min(min.x, -0.5f);
                min.y = -1.0f;
                max.x = std::max(max.x, 0.5f);
                break;
            case bottom:
                min.x = std::min(min.x, -0.5f);
                max.x = std::max(max.x, 0.5f);
                max.y = 1.0f;
                break;
            case left:
                min.x = -1.0f;
                min.y = std::min(min.y, -0.5f);
                max.y = std::max(max.y, 0.5f);
                break;
            case right:
                min.y = std::min(min.y, -0.5f);
                max.x = 1.0f;
                max.y = std::max(max.y, 0.5f);
                break;
            case top_left:
                min.x = -1.0f;
                min.y = -1.0f;
                break;
            case top_right:
                min.y = -1.0f;
                max.x = 1.0f;
                break;
            case bottom_left:
                min.x = -1.0f;
                max.y = 1.0f;
                break;
            case bottom_right:
                max.x = 1.0f;
                max.y = 1.0f;
                break;
            }
        }
        // TODO add AABB constructor to pass min/max
        return isect2d::AABB<glm::vec2>(min.x * size.x, min.y * size.y,
                                        max.x * size.x, max.y * size.y);
    }
};

bool anchor(const std::string& _transform, Anchor& _out);
bool placement(const std::string& placement, Placement& out);

glm::vec2 anchorDirection(Anchor _anchor);

} // LabelProperty

namespace TextLabelProperty {

enum class Transform {
    none,
    capitalize,
    uppercase,
    lowercase,
};

enum class Align {
    none   = -1,
    right  = 0,
    left   = 1,
    center = 2,
};

bool transform(const std::string& _transform, Transform& _out);
bool align(const std::string& _transform, Align& _out);
Align alignFromAnchor(LabelProperty::Anchor _anchor);

} // TextLabelProperty
} // Tangram
