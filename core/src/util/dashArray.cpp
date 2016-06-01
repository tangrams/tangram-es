#include "dashArray.h"

namespace Tangram {

std::vector<unsigned int> DashArray::render(std::vector<int> _pattern,
    unsigned int _dashColor, unsigned int _backgroundColor)
{
    std::vector<unsigned int> dashArray;
    if (_pattern.size() % 2 == 1) {
        _pattern.insert(_pattern.end(), _pattern.begin(), _pattern.end());
    }

    bool dash = true;
    for (int segment : _pattern) {
        for (int i = 0; i < segment; ++i) {
            dashArray.push_back(dash ? _dashColor : _backgroundColor);
        }
        dash = !dash;
    }

    return dashArray;
}

}