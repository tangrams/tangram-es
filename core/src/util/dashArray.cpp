#include "util/dashArray.h"

#include <cmath>

namespace Tangram {

std::vector<unsigned int> DashArray::render(std::vector<float> _pattern, float _dashScale,
    unsigned int _dashColor, unsigned int _backgroundColor)
{
    std::vector<unsigned int> dashArray;
    if (_pattern.size() % 2 == 1) {
        _pattern.insert(_pattern.end(), _pattern.begin(), _pattern.end());
    }

    bool dash = true;
    for (auto& pat : _pattern) {
        int segment = std::floor(pat * _dashScale);
        for (int i = 0; i < segment; ++i) {
            dashArray.push_back(dash ? _dashColor : _backgroundColor);
        }
        dash = !dash;
    }

    return dashArray;
}

}
