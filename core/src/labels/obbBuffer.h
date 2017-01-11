#pragma once

#include "obb.h"
#include "util/types.h"

namespace Tangram {

struct OBBBuffer {

    OBBBuffer(std::vector<isect2d::OBB<glm::vec2>>& _obbs, Range& _range)
        : obbs(_obbs), range(_range) {
        if (!range.length) {
            range.start = obbs.size();
        }
    }

    auto begin() { return obbs.begin() + range.start; }
    auto end() { return obbs.begin() + range.end(); }

    void clear() {
        range.length = 0;
        obbs.resize(range.start);
    }

    void append(isect2d::OBB<glm::vec2> _obb) {
        obbs.push_back(_obb);
        range.length += 1;
    }

private:
    std::vector<isect2d::OBB<glm::vec2>>& obbs;
    Range& range;

};

}
