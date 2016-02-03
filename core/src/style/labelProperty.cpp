#include "labelProperty.h"
#include <map>

namespace Tangram {
namespace LabelProperty {

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

} // TextLabelProperty
} // Tangram
