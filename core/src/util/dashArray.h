#pragma once

#include <vector>

namespace Tangram {

struct DashArray {
    static std::vector<unsigned int> render(std::vector<float> _pattern, float _dashScale,
        unsigned int _dashColor = 0xffffffff,
        unsigned int _backgroundColor = 0x00000000);
};

}
