#pragma once

#include <vector>

namespace Tangram {

struct DashArray {
    static std::vector<unsigned int> render(std::vector<int> _pattern,
        unsigned int _dashColor = 0xffffffff,
        unsigned int _backgroundColor = 0x00000000);
};

}