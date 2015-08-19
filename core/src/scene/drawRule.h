#pragma once

#include "util/variant.h"

#include <string>
#include <utility>
#include <vector>

#include "builders.h" // for Cap/Join types
#include "csscolorparser.hpp"

using Color = CSSColorParser::Color;

namespace Tangram {

enum class StyleParamKey : uint8_t {
    none, order, color, width, cap, join, outline_color, outline_width, outline_cap, outline_join,
};

struct StyleParam {
    using Value = variant<none_type, std::string, Color, CapTypes, JoinTypes, int32_t, float>;

    StyleParam() {}
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

    bool getValue(StyleParamKey _key, std::string& _str) const;
    bool getValue(StyleParamKey _key, float& value) const;
    bool getValue(StyleParamKey _key, int32_t& value) const;
    bool getColor(StyleParamKey _key, uint32_t& value) const;
    bool getLineCap(StyleParamKey _key, CapTypes& value) const;
    bool getLineJoin(StyleParamKey _key, JoinTypes& value) const;

    bool operator<(const DrawRule& _rhs) const;
    int compare(const DrawRule& _rhs) const { return style.compare(_rhs.style); }
};

}
