#pragma once

#include "util/variant.h"

#include <string>
#include <utility>
#include <vector>
#include <tuple>

#include "builders.h" // for Cap/Join types
#include "csscolorparser.hpp"
#include "platform.h"

using Color = CSSColorParser::Color;
using Extrusion = std::pair<float, float>;

namespace Tangram {

enum class StyleParamKey : uint8_t {
    none, order, extrude, color, width, cap, join, outline_color, outline_width, outline_cap, outline_join,
    font_name, font_weight, font_face, font_size, font_fill, font_stroke, font_stroke_color, font_stroke_width, font_capitalized,
    font_visible,
};

struct StyleParam {
    using Value = variant<none_type, bool, std::string, Color, CapTypes, JoinTypes, Extrusion, int32_t, float>;

    StyleParam() : key(StyleParamKey::none), value(none_type{}) {};
    StyleParam(const std::string& _key, const std::string& _value);
    StyleParam(StyleParamKey _key, std::string _value) : key(_key), value(std::move(_value)){}

    StyleParamKey key;
    Value value;
    bool operator<(const StyleParam& _rhs) const { return key < _rhs.key; }
    bool valid() const { return !value.is<none_type>(); }
    operator bool() const { return valid(); }

    std::string toString() const;
};

struct DrawRule {

    static Color parseColor(const std::string& _color);

    std::string style;
    std::vector<StyleParam> parameters;

    DrawRule(const std::string& _style, const std::vector<StyleParam>& _parameters);

    DrawRule merge(DrawRule& _other) const;
    std::string toString() const;

    const StyleParam& findParameter(StyleParamKey _key) const;

    template<typename T>
    bool get(StyleParamKey _key, T& _value) const {
        auto& param = findParameter(_key);
        if (!param) { return false; }
        if (!param.value.is<T>()) {
            logMsg("Error: wrong type '%d'for StyleParam '%d' \n",
                   param.value.which(), _key);
            return false;
        }
        _value = param.value.get<T>();
        return true;
    }

    bool operator<(const DrawRule& _rhs) const;
    int compare(const DrawRule& _rhs) const { return style.compare(_rhs.style); }
};

}
