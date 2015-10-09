#pragma once

#include "scene/styleParam.h"

#include <string>
#include <vector>

namespace Tangram {

class StyleContext;

struct DrawRule {

    std::string styleName;
    int styleId;
    std::vector<StyleParam> parameters;

    DrawRule(std::string _styleName, int _styleId, const std::vector<StyleParam>& _parameters,
             bool _sorted = false);

    //Merge properties of _other and retain self
    DrawRule merge(DrawRule& _other) const;
    std::string toString() const;

    bool eval(const StyleContext& _ctx);

    const std::string& getStyleName() const;

    const StyleParam& findParameter(StyleParamKey _key) const;

    bool isJSFunction(StyleParamKey _key) const;

    template<typename T>
    bool get(StyleParamKey _key, T& _value) const {
        auto& param = findParameter(_key);
        if (!param) { return false; }
        if (!param.value.is<T>()) {
            return false;
        }
        _value = param.value.get<T>();
        return true;
    }
    bool contains(StyleParamKey _key) const {
        return findParameter(_key) != false;
    }

    bool operator<(const DrawRule& _rhs) const;

private:
    void logGetError(StyleParamKey _expectedKey, const StyleParam& _param);
};

}
