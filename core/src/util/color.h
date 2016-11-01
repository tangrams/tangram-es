#pragma once
#include <cstdint>
#include "glm/vec4.hpp"

namespace Tangram {

struct Color {

    union {
        struct {
            uint8_t r, g, b, a;
        };
        uint32_t abgr = 0;
    };

    Color() = default;
    Color(uint32_t _abgr) : abgr(_abgr) {}
    Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) : r(_r), g(_g), b(_b), a(_a) {}

    glm::ivec4 asIVec4() {
        return glm::ivec4(r, g, b, a);
    }

    static Color mix(const Color& _x, const Color& _y, float _a) {
        return Color(
            _x.r * (1 - _a) + _y.r * _a,
            _x.g * (1 - _a) + _y.g * _a,
            _x.b * (1 - _a) + _y.b * _a,
            _x.a * (1 - _a) + _y.a * _a
        );
    }

};

}
