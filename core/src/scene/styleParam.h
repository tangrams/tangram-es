#pragma once

#include "labels/labelProperty.h"
#include "util/variant.h"

#include "glm/vec2.hpp"
#include <string>
#include <vector>

namespace Tangram {

struct Stops;

enum class StyleParamKey : uint8_t {
    text_align,
    anchor,
    angle,
    buffer,
    text_anchor,
    cap,
    collide,
    text_collide,
    color,
    extrude,
    flat,
    text_buffer,
    text_font_family,
    text_font_fill,
    text_font_size,
    text_font_stroke_color,
    text_font_stroke_width,
    text_font_style,
    text_font_weight,
    interactive,
    text_interactive,
    join,
    miter_limit,
    none,
    offset,
    text_offset,
    order,
    outline_cap,
    outline_color,
    outline_join,
    outline_miter_limit,
    outline_order,
    outline_width,
    outline_style,
    outline_visible,
    placement,
    placement_spacing,
    placement_min_length_ratio,
    priority,
    repeat_distance,
    repeat_group,
    text_order,
    text_priority,
    text_repeat_distance,
    text_repeat_group,
    size,
    sprite,
    sprite_default,
    style,
    text_source,
    text_wrap,
    tile_edges,
    text_transform,
    transition_hide_time,
    transition_selected_time,
    transition_show_time,
    text_optional,
    text_transition_hide_time,
    text_transition_selected_time,
    text_transition_show_time,
    text_visible,
    visible,
    width,
    point_text,
    NUM_ELEMENTS
};

constexpr size_t StyleParamKeySize = static_cast<size_t>(StyleParamKey::NUM_ELEMENTS);

enum class Unit { pixel, milliseconds, meter, seconds };

static inline std::string unitString(Unit unit) {
    switch(unit) {
        case Unit::pixel: return "pixel";
        case Unit::milliseconds: return "milliseconds";
        case Unit::meter: return "meter";
        case Unit::seconds: return "seconds";
        default: return "undefined";
    }
}

template <typename T>
struct UnitVec {
    T value = T(0.0);
    static constexpr int size = sizeof(value)/sizeof(value[0]);
    Unit units[size];

    UnitVec() {
        for (int i = 0; i < size; ++i) {
            units[i] = Unit::meter;
        }
    }
};

struct StyleParam {

    struct ValueUnitPair {
        ValueUnitPair() = default;
        ValueUnitPair(float _value, Unit _unit)
            : value(_value), unit(_unit) {}

        float value = 0.f;
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

    using Value = variant<Undefined, none_type, bool, float, uint32_t, std::string, glm::vec2,
                            Width, LabelProperty::Placement, LabelProperty::Anchors>;

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

    // values within _value string parameter must be delimited by ','
    static bool parseVec2(const std::string& _value, const std::vector<Unit>& _allowedUnits, UnitVec<glm::vec2>& _vec2);
    static bool parseVec3(const std::string& _value, const std::vector<Unit>& _allowedUnits, UnitVec<glm::vec3>& _vec3);

    static int parseValueUnitPair(const std::string& _value, size_t start,
                                  StyleParam::ValueUnitPair& _result);

    static Value parseString(StyleParamKey key, const std::string& _value);

    static bool isColor(StyleParamKey _key);
    static bool isSize(StyleParamKey _key);
    static bool isWidth(StyleParamKey _key);
    static bool isOffsets(StyleParamKey _key);
    static bool isNumberType(StyleParamKey _key);
    static bool isFontSize(StyleParamKey _key);
    static bool isRequired(StyleParamKey _key);

    static const std::vector<Unit>& unitsForStyleParam(StyleParamKey _key);

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
