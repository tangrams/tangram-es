#pragma once

#include <string>
#include "util/util.h"
#include "glm/vec2.hpp"

namespace Tangram {

namespace LabelProperty {

enum Anchor {
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

bool anchor(const std::string& _transform, Anchor& _out);

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
    none,
    right,
    left,
    center,
};

bool transform(const std::string& _transform, Transform& _out);
bool align(const std::string& _transform, Align& _out);
Align alignFromAnchor(LabelProperty::Anchor _anchor);

} // TextLabelProperty
} // Tangram
