#pragma once
#include <string>
#include <utility>
#include <vector>

namespace Tangram {

struct StyleParam {
    std::string key;
    std::string value;
    bool operator<(const StyleParam& _rhs) const { return key < _rhs.key; }
};

struct DrawRule {

    std::string style;
    std::vector<StyleParam> parameters;

    DrawRule(const std::string& _style, const std::vector<StyleParam>& _parameters);

    DrawRule merge(DrawRule& _other) const;
    std::string toString() const;
    bool findParameter(const std::string& _key, std::string* _out) const;
    bool operator<(const DrawRule& _rhs) const;

};

}