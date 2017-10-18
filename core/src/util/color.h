#pragma once
#include <cstdint>

namespace Tangram {

// We have two structs to represent color:
struct Color;
struct ColorF;

// Color represents a 32-bit color, with 8 bits per color channel for 'r', 'g',
// 'b', and 'a'. The color can also be accessed as a single 32-bit integer with
// all channels packed together as 'abgr'. A Color can be converted to a ColorF
// without losing precision.
struct Color {

    // This union allows the color components to be accessed either as separate
    // bytes or as an integer combining the four bytes. On a big-endian system
    // the integer will combine the components in 'rgba' order instead.
    union {
        struct {
            uint8_t r, g, b, a;
        };
        uint32_t abgr = 0;
    };

    Color() = default;
    Color(uint32_t _abgr) : abgr(_abgr) {}
    Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
        : r(_r), g(_g), b(_b), a(_a) {}

    bool operator==(const Color& other) {
        return abgr == other.abgr;
    }

    inline ColorF toColorF();

    static Color mix(const Color& _x, const Color& _y, float _a) {
        return Color(
            _x.r * (1 - _a) + _y.r * _a,
            _x.g * (1 - _a) + _y.g * _a,
            _x.b * (1 - _a) + _y.b * _a,
            _x.a * (1 - _a) + _y.a * _a
        );
    }

};

// ColorF represents a color with floating point channel values for 'r', 'g',
// 'b', and 'a'. A ColorF can be converted to a Color but may lose precision.
struct ColorF {

    float r = 0, g = 0, b = 0, a = 0;

    ColorF() = default;
    ColorF(float _r, float _g, float _b, float _a)
        : r(_r), g(_g), b(_b), a(_a) {}

    bool operator==(const ColorF& other) {
        return r == other.r && (g == other.g && (b == other.b && a == other.a));
    }

    inline Color toColor();

    static ColorF mix(const ColorF& _x, const ColorF& _y, float _a) {
        return ColorF(
            _x.r * (1 - _a) + _y.r * _a,
            _x.g * (1 - _a) + _y.g * _a,
            _x.b * (1 - _a) + _y.b * _a,
            _x.a * (1 - _a) + _y.a * _a
        );
    }

};

ColorF Color::toColorF() {
    return ColorF(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
}

Color ColorF::toColor() {
    return Color(255 * r, 255 * g, 255 * b, 255 * a);
}

} // namespace Tangram
