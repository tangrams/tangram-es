#pragma once

#include "scene/styleParam.h"

#include <vector>
#include <deque>

namespace Tangram {

class Style;
class Scene;
class Tile;
class StyleContext;
class SceneLayer;
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
            return false;
        }
        _value = param.value.get<T>();
        return true;
    }

    bool contains(StyleParamKey _key) const {
        return findParameter(_key) != false;
    }

private:
    void logGetError(StyleParamKey _expectedKey, const StyleParam& _param) const;

};

struct Styling {

    /* Determine and apply DrawRules for a @_feature and add
     * the result to @_tile
     */
    void apply(const Feature& _feature, const Scene& _scene,
               const SceneLayer& _sceneLayer,
               StyleContext& _ctx, Tile& _tile);

    // internal
    bool match(const Feature& _feature, const SceneLayer& _layer, StyleContext& _ctx);

    // internal
    void mergeRules(const std::vector<StaticDrawRule>& rules);

    // Reusable 'styles' and 'processQ'
    std::vector<DrawRule> styles;
    // NB: Minimal memory usage of deque:
    // http://info.prelert.com/blog/stl-container-memory-usage
    // libdc++   4096 + 8 bytes heap
    // libstdc++ 512 + 64 bytes heap
    std::deque<std::vector<SceneLayer>::const_iterator> processQ;

};

}
