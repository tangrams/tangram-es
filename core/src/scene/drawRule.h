#pragma once
#include <string>
#include <utility>
#include <vector>

#include "builders.h" // for Cap/Join types

namespace Tangram {

enum class StyleParamKey : uint8_t {
    order, color, width, cap, join, outline_color, outline_width, outline_cap, outline_join,
};

struct StyleParam {
    StyleParam() {}
    StyleParam(const std::string& _key, const std::string& _value);

    StyleParamKey key;
    std::string value;
    bool operator<(const StyleParam& _rhs) const { return key < _rhs.key; }
    int compare(const StyleParam& _rhs) const {
        int d = static_cast<int>(key) - static_cast<int>(_rhs.key);
        return d < 0 ? -1 : d > 0 ? 1 : 0;
    }
    bool valid() const { return !value.empty(); }
    operator bool() const { return valid(); }
};

struct DrawRule {

    static uint32_t parseColor(const std::string& _color);

    std::string style;
    std::vector<StyleParam> parameters;

    DrawRule(const std::string& _style, const std::vector<StyleParam>& _parameters);

    DrawRule merge(DrawRule& _other) const;
    std::string toString() const;

    const StyleParam& findParameter(StyleParamKey _key) const;

    bool findParameter(const char* _key, std::string& _str) const;

    bool getValue(StyleParamKey _key, float& value) const;
    bool getValue(StyleParamKey _key, int32_t& value) const;
    bool getColor(StyleParamKey _key, uint32_t& value) const;
    bool getLineCap(StyleParamKey _key, CapTypes& value) const;
    bool getLineJoin(StyleParamKey _key, JoinTypes& value) const;

    bool operator<(const DrawRule& _rhs) const;
    int compare(const DrawRule& _rhs) const { return style.compare(_rhs.style); }
};

}
