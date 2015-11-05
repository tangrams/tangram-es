#pragma once

#include "text/fontContext.h"

namespace Tangram {
namespace Text {

enum class Transform {
    none,
    capitalize,
    uppercase,
    lowercase,
};

enum class Align : char {
    right,
    left,
    center,
};

enum class Anchor {
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

template <class M, class T>
inline bool tryFind(M& _map, const std::string& _transform, T& _out) {
    auto it = _map.find(_transform);
    if (it != _map.end()) {
        _out = it->second;
        return true;
    }
    return false;
}

bool transform(const std::string& _transform, Transform& _out);
bool align(const std::string& _transform, Align& _out);
bool anchor(const std::string& _transform, Anchor& _out);

}
}
