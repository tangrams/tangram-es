#pragma once

#include "util/variant.h"
#include "glm/vec2.hpp"
#include <string>
#include <vector>

namespace Tangram {

struct Stops;

enum class StyleParamKey : uint8_t {
    cap,
    color,
    extrude,
    font_family,
    font_fill,
    font_size,
    font_stroke,
    font_stroke_color,
    font_stroke_width,
    font_style,
    font_weight,
    join,
    none,
    offset,
    order,
    outline_cap,
    outline_color,
    outline_join,
    outline_width,
    priority,
    sprite,
    sprite_default,
    size,
    text_source,
    transform,
    visible,
    width,
};

struct StyleParam {
    using Value = variant<none_type, bool, float, uint32_t, std::string, glm::vec2>;

    StyleParam() : key(StyleParamKey::none), value(none_type{}) {};
    StyleParam(const std::string& _key, const std::string& _value);

    StyleParam(StyleParamKey _key, std::string _value) :
        key(_key),
        value(std::move(_value)) {}

    StyleParamKey key;
    Value value;
    Stops* stops = nullptr;
    int32_t function = -1;

    bool operator<(const StyleParam& _rhs) const { return key < _rhs.key; }
    bool valid() const { return !value.is<none_type>(); }
    operator bool() const { return valid(); }

    std::string toString() const;

    /* parse a font size (in em, pt, %) and give the appropriate size in pixel */
    static bool parseFontSize(const std::string& _size, float& _pxSize);

    static uint32_t parseColor(const std::string& _color);

    static bool parseVec2(const std::string& _value, const std::vector<std::string>& _allowedUnits, glm::vec2& _vec2);

    static Value parseString(StyleParamKey key, const std::string& _value);

    static bool isColor(const std::string& _keyName);
};

}
