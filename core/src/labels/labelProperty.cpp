#include "labels/labelProperty.h"

#include <map>

namespace Tangram {
namespace LabelProperty {

const std::map<std::string, Placement> s_PlacementMap = {
    {"vertex", Placement::vertex},
    {"midpoint", Placement::midpoint},
    {"spaced", Placement::spaced},
    {"centroid", Placement::centroid},
};

const std::map<std::string, Anchor> s_AnchorMap = {
    {"center", Anchor::center},
    {"top", Anchor::top},
    {"bottom", Anchor::bottom},
    {"left", Anchor::left},
    {"right", Anchor::right},
    {"top-left", Anchor::top_left},
    {"top-right", Anchor::top_right},
    {"bottom-left", Anchor::bottom_left},
    {"bottom-right", Anchor::bottom_right},
};

bool anchor(const std::string& _anchor, Anchor& _out) {
    return tryFind(s_AnchorMap, _anchor, _out);
}

bool placement(const std::string& placement, Placement& out) {
    return tryFind(s_PlacementMap, placement, out);
}

glm::vec2 anchorDirection(Anchor _anchor) {
    glm::vec2 direction;

    switch (_anchor) {
        case top: direction = glm::vec2(0.0, -1.0); break;
        case bottom: direction = glm::vec2(0.0, 1.0); break;
        case left: direction = glm::vec2(-1.0, 0.0); break;
        case right: direction = glm::vec2(1.0, 0.0); break;
        case top_left: direction = glm::vec2(-1.0, -1.0); break;
        case top_right: direction = glm::vec2(1.0, -1.0); break;
        case bottom_left: direction = glm::vec2(-1.0, 1.0); break;
        case bottom_right: direction = glm::vec2(1.0, 1.0); break;
        case center: direction = glm::vec2(0.0, 0.0); break;
    }

    return direction;
}

} // LabelProperty

namespace TextLabelProperty {

const std::map<std::string, Transform> s_TransformMap = {
    {"none", Transform::none},
    {"capitalize", Transform::capitalize},
    {"uppercase", Transform::uppercase},
    {"lowercase", Transform::lowercase},
};

const std::map<std::string, Align> s_AlignMap = {
    {"right", Align::right},
    {"left", Align::left},
    {"center", Align::center},
};

bool transform(const std::string& _transform, Transform& _out) {
    return tryFind(s_TransformMap, _transform, _out);
}

bool align(const std::string& _align, Align& _out) {
    return tryFind(s_AlignMap, _align, _out);
}

Align alignFromAnchor(LabelProperty::Anchor _anchor) {
    switch(_anchor) {
        case LabelProperty::Anchor::top_left:
        case LabelProperty::Anchor::left:
        case LabelProperty::Anchor::bottom_left:
            return TextLabelProperty::Align::right;
        case LabelProperty::Anchor::top_right:
        case LabelProperty::Anchor::right:
        case LabelProperty::Anchor::bottom_right:
            return TextLabelProperty::Align::left;
        case LabelProperty::Anchor::top:
        case LabelProperty::Anchor::bottom:
        case LabelProperty::Anchor::center:;
    }
    return TextLabelProperty::Align::center;
}

} // TextLabelProperty
} // Tangram
