#pragma once

#include <string>

namespace Tangram {

template <class M, class T>
inline bool tryFind(M& _map, const std::string& _transform, T& _out) {
    auto it = _map.find(_transform);
    if (it != _map.end()) {
        _out = it->second;
        return true;
    }
    return false;
}

namespace LabelProperty {

enum Anchor {
    center,
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

} // LabelProperty

namespace TextLabelProperty {

enum Transform {
    none,
    capitalize,
    uppercase,
    lowercase,
};

enum Align {
    right,
    left,
    center,
};

bool transform(const std::string& _transform, Transform& _out);
bool align(const std::string& _transform, Align& _out);

} // TextLabelProperty
} // Tangram
