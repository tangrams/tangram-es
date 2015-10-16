#pragma once

#include "util/variant.h"
#include "glm/vec2.hpp"
#include <string>
#include <vector>

namespace Tangram {

struct Stops;

enum class StyleParamKey : uint8_t {
    align,
    anchor,
    cap,
    centroid,
    collide,
    color,
    extrude,
    font_family,
    font_fill,
    font_size,
    font_stroke_color,
    font_stroke_width,
    font_style,
    font_weight,
    interactive,
    join,
    none,
    offset,
    order,
    outline_cap,
    outline_color,
    outline_join,
    outline_order,
    outline_width,
    priority,
    size,
    sprite,
    sprite_default,
    style,
    text_source,
    text_wrap,
    transform,
    transition_hide_time,
    transition_selected_time,
    transition_show_time,
    visible,
    width,
};

// UPDATE WITH StyleParamKey CHANGES!
constexpr size_t StyleParamKeySize = static_cast<size_t>(StyleParamKey::width)+1;

enum class Unit { pixel, milliseconds, meter, seconds };

struct StyleParam {

    struct ValueUnitPair {
        ValueUnitPair() = default;
        ValueUnitPair(float _value, Unit _unit)
            : value(_value), unit(_unit) {}

        float value;
        Unit unit = Unit::meter;

        bool isMeter() const { return unit == Unit::meter; }
        bool isPixel() const { return unit == Unit::pixel; }
        bool isSeconds() const { return unit == Unit::seconds; }
        bool isMilliseconds() const { return unit == Unit::milliseconds; }

    };
    struct Width : ValueUnitPair {

        Width() = default;
        Width(float _value) :
            ValueUnitPair(_value, Unit::meter) {}
        Width(float _value, Unit _unit)
            : ValueUnitPair(_value, _unit) {}

        Width(ValueUnitPair& _other) :
            ValueUnitPair(_other) {}

        bool operator==(const Width& _other) const {
            return value == _other.value && unit == _other.unit;
        }
        bool operator!=(const Width& _other) const {
            return value != _other.value || unit != _other.unit;
        }
    };

    using Value = variant<none_type, bool, float, uint32_t, std::string, glm::vec2, Width>;

    StyleParam() :
        key(StyleParamKey::none),
        value(none_type{}) {};

    StyleParam(const std::string& _key, const std::string& _value);

    StyleParam(StyleParamKey _key, std::string _value) :
        key(_key),
        value(std::move(_value)) {}

    StyleParam(StyleParamKey _key, Stops* _stops) :
        key(_key),
        value(none_type{}),
        stops(_stops) {
    }

    StyleParamKey key;
    Value value;
    Stops* stops = nullptr;
    int32_t function = -1;

    bool operator<(const StyleParam& _rhs) const { return key < _rhs.key; }
    bool valid() const { return !value.is<none_type>() || stops != nullptr || function >= 0; }
    operator bool() const { return valid(); }

    std::string toString() const;

    /* parse a font size (in em, pt, %) and give the appropriate size in pixel */
    static bool parseFontSize(const std::string& _size, float& _pxSize);

    static uint32_t parseColor(const std::string& _color);

    static bool parseTime(const std::string& _value, float& _time);

    static bool parseVec2(const std::string& _value, const std::vector<Unit> _allowedUnits, glm::vec2& _vec2);

    static int parseValueUnitPair(const std::string& _value, size_t start,
                                  StyleParam::ValueUnitPair& _result);

    static Value parseString(StyleParamKey key, const std::string& _value);

    static bool isColor(StyleParamKey _key);
    static bool isWidth(StyleParamKey _key);
    static bool isRequired(StyleParamKey _key);

    static StyleParamKey getKey(const std::string& _key);
};

}
