#pragma once

#include "styleParam.h"
#include "platform.h"

#include <vector>

namespace Tangram {

class Style;
class Scene;
class Tile;
class StyleContext;
struct Feature;
struct StaticDrawRule;

struct DrawRule {

    // Reference to original StyleParams
    const StyleParam* params[StyleParamKeySize];

    // Evaluated params for stops and functions
    StyleParam evaluated[StyleParamKeySize];

    std::string styleName;
    int styleId;

    bool isJSFunction(StyleParamKey _key) const {
        auto& param = findParameter(_key);
        if (!param) {
            return false;
        }
        return param.function >= 0;
    }

    const std::string& getStyleName() const;

    const StyleParam& findParameter(StyleParamKey _key) const;

    template<typename T>
    bool get(StyleParamKey _key, T& _value) const {
        auto& param = findParameter(_key);
        if (!param) { return false; }
        if (!param.value.is<T>()) {
            LOGE("wrong type '%d'for StyleParam '%d'",
                   param.value.which(), _key);
            return false;
        }
        _value = param.value.get<T>();
        return true;
    }

    bool contains(StyleParamKey _key) const {
        return findParameter(_key) != false;
    }
};

struct Styling {
    std::vector<DrawRule> styles;

    void apply(Tile& _tile, const Feature& _feature, const Scene& _scene, StyleContext& _ctx);

    void mergeRules(const std::vector<StaticDrawRule>& rules);

};

}
