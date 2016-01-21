#pragma once

#include "scene/styleParam.h"

#include <vector>
#include <set>
#include <bitset>

namespace Tangram {

struct Feature;
class Tile;
class Scene;
class SceneLayer;
class StyleContext;

/*
 * A draw rule is a named collection of style parameters. When a draw rule is found to match a
 * feature, the feature's geometry is built into drawable buffers using a style determined from the
 * rule with the parameters contained in the rule.
 *
 * Draw rules are represented in two ways: by a DrawRuleData and by a DrawRule.
 *
 * DrawRuleData represents a named set of style parameters *as they are written in the layer*.
 * This is different from the set of style parameters that is applied to a feature after matching
 * and merging; the merged set of style parameters is represented by DrawRule.
 *
 * DrawRule is a temporary object and only contains style parameters that are defined in at least
 * one DrawRuleData, so it can safely reference any needed parameters by pointers to the original.
 * However, style parameters that need to be evaluated per-feature must also have space to store
 * their evaluated values.
 *
 * When matching and merging draw rules, the name of the rule is frequently copied and compared. To
 * make this process faster, each string used as the name of a draw rule is assigned to an integer
 * index within the scene object and then stored as the id of a draw rule.
 */

struct DrawRuleData {

    std::vector<StyleParam> parameters;
    std::string name;
    int id;

    DrawRuleData(std::string _name, int _id,
                 const std::vector<StyleParam>& _parameters);

    std::string toString() const;

};

struct DrawRule {

    std::bitset<StyleParamKeySize> active = { 0 };

    struct Param {
        const StyleParam* param;
        // Layer name
        const char* name;
        // Layer depth
        size_t depth;
    };

    Param params[StyleParamKeySize];

    const std::string* styleName = nullptr;
    int styleId;

    DrawRule(const DrawRuleData& _ruleData, const SceneLayer& _layer);

    void merge(const DrawRuleData& _ruleData, const SceneLayer& _layer);

    bool isJSFunction(StyleParamKey _key) const;

    bool contains(StyleParamKey _key) const;

    const std::string& getStyleName() const;

    const char* getLayerName(StyleParamKey _key) const;

    size_t getParamSetHash() const;

    const StyleParam& findParameter(StyleParamKey _key) const;

    template<typename T>
    bool get(StyleParamKey _key, T& _value) const {
        if (auto& param = findParameter(_key)) {
            return StyleParam::Value::visit(param.value,
                                            prop_visitor<T>{ _value });
        }
        return false;
    }

    template<typename T>
    const T* get(StyleParamKey _key) const {
        if (auto& param = findParameter(_key)) {
            return StyleParam::Value::visit(param.value,
                                            prop_visitor_ptr<T>{});
        }
        return nullptr;
    }

private:
    void logGetError(StyleParamKey _expectedKey, const StyleParam& _param) const;

    template<typename T>
    struct prop_visitor {
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
    struct prop_visitor_ptr {
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

class DrawRuleMergeSet {

public:
    /* Determine and apply DrawRules for a @_feature and add
     * the result to @_tile
     */
    void apply(const Feature& _feature, const Scene& _scene,
               const SceneLayer& _sceneLayer,
               StyleContext& _ctx, Tile& _tile);

    // internal
    bool match(const Feature& _feature, const SceneLayer& _layer, StyleContext& _ctx);

    // internal
    void mergeRules(const SceneLayer& _layer);

    auto& matchedRules() { return m_matchedRules; }

private:
    // Reusable containers 'matchedRules' and 'queuedLayers'
    std::vector<DrawRule> m_matchedRules;
    std::vector<const SceneLayer*> m_queuedLayers;

    // Container for dynamically-evaluated parameters
    StyleParam m_evaluated[StyleParamKeySize];

};

}
