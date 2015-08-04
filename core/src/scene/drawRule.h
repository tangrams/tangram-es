#pragma once
#include <string>
#include <utility>
#include <vector>

#include "builders.h" // for Cap/Join types

namespace Tangram {

struct StyleParam {
    StyleParam() {}
    StyleParam(std::string _key, std::string _value) : key(std::move(_key)), value(std::move(_value)){}

    std::string key;
    std::string value;
    bool operator<(const StyleParam& _rhs) const { return key < _rhs.key; }
    int compare(const StyleParam& _rhs) const { return key.compare(_rhs.key); }
    bool valid() const { return !key.empty(); }
    operator bool() const { return valid(); }

    static const StyleParam NONE;
};

struct DrawRule {

    static uint32_t parseColor(const std::string& _color);

    std::string style;
    std::vector<StyleParam> parameters;

    DrawRule(const std::string& _style, const std::vector<StyleParam>& _parameters);

    DrawRule merge(DrawRule& _other) const;
    std::string toString() const;

    const StyleParam& findParameter(const char* _key) const;

    bool findParameter(const char* _key, std::string& _str) const {
        auto& param = findParameter(_key);
        if (param) {
            _str = param.value;
            return false;
        }
        return false;
    };

    bool getValue(const char* _key, float& value) const;
    bool getValue(const char* _key, int32_t& value) const;
    bool getColor(const char* _key, uint32_t& value) const;
    bool getLineCap(const char* _key, CapTypes& value) const;
    bool getLineJoin(const char* _key, JoinTypes& value) const;

    bool operator<(const DrawRule& _rhs) const;
    int compare(const DrawRule& _rhs) const { return style.compare(_rhs.style); }
};

}
