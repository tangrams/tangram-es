#pragma once

#include "labels/labelProperty.h"
#include "util/variant.h"

#include "glm/vec2.hpp"
#include <initializer_list>
#include <string>
#include <vector>

// Forward declaration
namespace YAML {
  class Node;
}

namespace Tangram {

struct Color;
struct Stops;

enum class StyleParamKey : uint8_t {
    anchor,
    angle,
    buffer,
    cap,
    collide,
    color,
    extrude,
    flat,
    interactive,
    join,
    miter_limit,
    none,
    offset,
    order,
    outline_cap,
    outline_color,
    outline_join,
    outline_miter_limit,
    outline_order,
    outline_style,
    outline_visible,
    outline_width,
    placement,
    placement_min_length_ratio,
    placement_spacing,
    point_text,
    priority,
    repeat_distance,
    repeat_group,
    size,
    sprite,
    sprite_default,
    style,
    text_align,
    text_anchor,
    text_buffer,
    text_collide,
    text_font_family,
    text_font_fill,
    text_font_size,
    text_font_stroke_color,
    text_font_stroke_width,
    text_font_style,
    text_font_weight,
    text_interactive,
    text_max_lines,
    text_offset,
    text_optional,
    text_order,
    text_priority,
    text_repeat_distance,
    text_repeat_group,
    text_source,
    text_source_left,
    text_source_right,
    text_transform,
    text_transition_hide_time,
    text_transition_selected_time,
    text_transition_show_time,
    text_visible,
    text_wrap,
    texture,
    tile_edges,
    transition_hide_time,
    transition_selected_time,
    transition_show_time,
    visible,
    width,
    NUM_ELEMENTS
};

constexpr size_t StyleParamKeySize = static_cast<size_t>(StyleParamKey::NUM_ELEMENTS);

enum class Unit {
    none = 0,
    pixel,
    milliseconds,
    meter,
    seconds,
    percentage,
    sizeauto,
};

const std::string unitStrings[] = {
        "",
        "px",
        "ms",
        "m",
        "s",
        "%",
        "auto",
};

struct UnitSet {
    int bits = 0;
    constexpr UnitSet(std::initializer_list<Unit> units) {
        for (const auto u : units) {
            bits |= 1 << static_cast<int>(u);
        }
    }
    bool contains(Unit unit) {
        return (bits & (1 << static_cast<int>(unit))) != 0;
    }
};

static inline std::string unitToString(Unit unit) {
    return unitStrings[static_cast<int>(unit)];
}

static inline Unit stringToUnit(const std::string& string, size_t offset, size_t length) {
    auto count = sizeof(unitStrings) / sizeof(*unitStrings);
    for (size_t i = 0; i < count; i++) {
        if (string.compare(offset, length, unitStrings[i]) == 0) {
            return static_cast<Unit>(i);
        }
    }
    return Unit::none;
}

template <typename T>
struct UnitVec {
    T value = T(0.0);
    static constexpr int size = sizeof(value)/sizeof(value[0]);
    Unit units[size] = { Unit::none };
};

struct StyleParam {

    struct ValueUnitPair {
        ValueUnitPair() = default;
        ValueUnitPair(float _value, Unit _unit)
            : value(_value), unit(_unit) {}

        float value = 0.f;
        Unit unit = Unit::meter;

        bool isMeter() const { return unit == Unit::meter; }
        bool isPercentage() const { return unit == Unit::percentage; }
        bool isAuto() const { return unit == Unit::sizeauto; }
        bool isPixel() const { return unit == Unit::pixel; }
        bool isSeconds() const { return unit == Unit::seconds; }
        bool isMilliseconds() const { return unit == Unit::milliseconds; }

        bool operator==(const ValueUnitPair& _other) const {
            return value == _other.value && unit == _other.unit;
        }
        bool operator!=(const ValueUnitPair& _other) const {
            return value != _other.value || unit != _other.unit;
        }
    };

    struct Width : ValueUnitPair {

        Width() = default;
        Width(float _value) :
            ValueUnitPair(_value, Unit::meter) {}
        Width(float _value, Unit _unit)
            : ValueUnitPair(_value, _unit) {}

        Width(ValueUnitPair& _other) :
            ValueUnitPair(_other) {}
    };

    struct TextSource {
        std::vector<std::string> keys;
        bool operator==(const TextSource& _other) const {
            return keys == _other.keys;
        }
    };

    struct SizeValue {
        ValueUnitPair x = { NAN, Unit::pixel };
        ValueUnitPair y = { NAN, Unit::pixel };

        // Apply this size value for a point with the given default sprite size, in CSS pixels.
        // To apply no default sprite size, input a vector of NaN values.
        // If either dimension of the output is NaN, then there is no valid size result.
        glm::vec2 getSizePixels(glm::vec2 spriteSize) const {
            if (x.isPercentage()) {
                return spriteSize * (x.value * 0.01f);
            }
            if (x.isAuto() && y.isPixel()) {
                return glm::vec2(y.value * spriteSize.x / spriteSize.y, y.value);
            }
            if (x.isPixel() && y.isAuto()) {
                return glm::vec2(x.value, x.value * spriteSize.y / spriteSize.x);
            }
            if (x.isPixel() && y.isPixel()) {
                if (std::isnan(y.value)) {
                    if (std::isnan(x.value)) {
                        return spriteSize;
                    }
                    return glm::vec2(x.value);
                }
                return glm::vec2(x.value, y.value);
            }
            return glm::vec2(NAN);
        }

        bool operator==(const SizeValue& other) const {
            return x == other.x && y == other.y;
        }

        bool operator!=(const SizeValue& other) const {
            return !(*this == other);
        }
    };

    using Value = variant<none_type, Undefined, bool, float, uint32_t, std::string, glm::vec2, SizeValue, Width,
                          LabelProperty::Placement, LabelProperty::Anchors, TextSource>;

    StyleParam() :
        key(StyleParamKey::none),
        value(none_type{}) {}

    explicit StyleParam(StyleParamKey _key) :
        key(_key),
        value(none_type{}) {

    }

    explicit StyleParam(const std::string& _key) :
        key(getKey(_key)),
        value(none_type{}) {
    }

    StyleParam(const std::string& key, const YAML::Node& value);

    StyleParam(StyleParamKey _key, const YAML::Node& _value);

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

    // values within _value string parameter must be delimited by ','
//    static bool parseSize(const std::string& _value, uint8_t _allowedUnits, SizeValue& _vec2);
//    static bool parseVec2(const std::string& _value, uint8_t _allowedUnits, UnitVec<glm::vec2>& _vec2);
//    static bool parseVec3(const std::string& _value, uint8_t _allowedUnits, UnitVec<glm::vec3>& _vec3);

    static int parseSizeUnitPair(const std::string& _value, size_t start,
                                 StyleParam::ValueUnitPair& _result);
    static int parseValueUnitPair(const std::string& _value, size_t start,
                                  StyleParam::ValueUnitPair& _result);

    static Value parseString(StyleParamKey key, const std::string& _value);

    // WIP: Node-based value parsing.
    static Value parseNode(StyleParamKey key, const YAML::Node& node);

    static bool parseSize(const YAML::Node& node, UnitSet allowedUnits, SizeValue& result);
    static bool parseVec2(const YAML::Node& node, UnitSet allowedUnits, UnitVec<glm::vec2>& result);
    static bool parseVec3(const YAML::Node& node, UnitSet allowedUnits, UnitVec<glm::vec3>& result);
    static bool parseSizeUnitPair(const std::string& value, StyleParam::ValueUnitPair& result);
    static bool parseValueUnitPair(const std::string& value, StyleParam::ValueUnitPair& result);

    static bool parseColor(const YAML::Node& node, Color& result);

    static bool isColor(StyleParamKey _key);
    static bool isSize(StyleParamKey _key);
    static bool isWidth(StyleParamKey _key);
    static bool isOffsets(StyleParamKey _key);
    static bool isNumberType(StyleParamKey _key);
    static bool isFontSize(StyleParamKey _key);
    static bool isRequired(StyleParamKey _key);

//    static uint8_t unitsForStyleParam(StyleParamKey _key);

    static UnitSet unitSetForStyleParam(StyleParamKey key);

    static StyleParamKey getKey(const std::string& _key);

    static const std::string& keyName(StyleParamKey _key);

    template<typename T>
    struct visitor {
        using result_type = bool;
        T& out;
        bool operator()(const T& v) const {
            out = v;
            return true;
        }
        template<typename O>
        bool operator()(const O v) const {
            return false;
        }
    };
    template<typename T>
    struct visitor_ptr {
        using result_type = const T*;
        const T* operator()(const T& v) const {
            return &v;
        }
        template<typename O>
        const T* operator()(const O v) const {
            return nullptr;
        }
    };
};

}
