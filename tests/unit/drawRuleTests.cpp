#include "catch.hpp"

#include "scene/drawRule.h"
#include "scene/sceneLayer.h"
#include "platform.h"

#include <cstdio>
#include <algorithm>

using namespace Tangram;

// Functions to initialize DrawRule instances
const int dg1 = 0;
const int dg2 = 1;

std::vector<DrawRuleData> instance_a() {

    std::vector<StyleParam> params;
    params.emplace_back(StyleParamKey::order, std::string{"value_0a"});
    params.emplace_back(StyleParamKey::join, std::string{"value_4a"});
    params.emplace_back(StyleParamKey::color, std::string{"value_1a"});

    std::vector<DrawRuleData> rules;
    rules.emplace_back("dg1", dg1, std::move(params));
    return rules;
}

std::vector<DrawRuleData> instance_b() {

    std::vector<StyleParam> params;
    params.emplace_back(StyleParamKey::order, std::string{"value_0b"});
    params.emplace_back(StyleParamKey::width, std::string{"value_2b"});
    params.emplace_back(StyleParamKey::color, std::string{"value_1b"});
    params.emplace_back(StyleParamKey::cap, std::string{"value_3b"});
    params.emplace_back(StyleParamKey::style, std::string{"value_4b"});

    std::vector<DrawRuleData> rules;
    rules.emplace_back("dg1", dg1, std::move(params));
    return rules;
}

std::vector<DrawRuleData> instance_c() {

    std::vector<StyleParam> params = {};

    // changed from dg2 - styles will not be merged otherwise

    std::vector<DrawRuleData> rules;
    rules.emplace_back("dg1", dg1, std::move(params));
    return rules;
}

TEST_CASE("DrawRule correctly merges with another DrawRule", "[DrawRule]") {

    const SceneLayer layer_a = { "a", Filter(), instance_a(), {} };
    const SceneLayer layer_b = { "b", Filter(), instance_b(), {} };
    const SceneLayer layer_c = { "c", Filter(), instance_c(), {} };

    // For parameters contained in multiple rules, the parameter from the last rule
    // (by lexicographical order) should result.
    {
        DrawRuleMergeSet ruleSet;
        ruleSet.mergeRules(layer_a);

        REQUIRE(ruleSet.matchedRules().size() == 1);
        auto& merged_ab = ruleSet.matchedRules()[0];

        for (size_t i = 0; i < StyleParamKeySize; i++) {
            if (!merged_ab.active[i]) {
                continue;
            }
            auto* param = merged_ab.params[i].param;
            if (!param) {
                logMsg("param : none %d\n", i);
                continue;
            }
            logMsg("param : %s\n", param->toString().c_str());
        }

        ruleSet.mergeRules(layer_b);

        REQUIRE(ruleSet.matchedRules().size() == 1);

        // printf("rule_a:\n %s", rule_a.toString().c_str());
        // printf("rule_c:\n %s", rule_c.toString().c_str());
        // printf("merged_ac:\n %s", merged_ac.toString().c_str());
        for (size_t i = 0; i < StyleParamKeySize; i++) {
            if (!merged_ab.active[i]) {
                continue;
            }
            auto* param = merged_ab.params[i].param;
            if (!param) {
                logMsg("param : none %d\n", i);
                continue;
            }
            logMsg("param : %s\n", param->toString().c_str());
        }
        REQUIRE(merged_ab.findParameter(StyleParamKey::cap).key == StyleParamKey::cap);
        REQUIRE(merged_ab.findParameter(StyleParamKey::cap).value.get<std::string>() == "value_3b");
        REQUIRE(merged_ab.findParameter(StyleParamKey::color).key == StyleParamKey::color);
        REQUIRE(merged_ab.findParameter(StyleParamKey::color).value.get<std::string>() == "value_1b");
        REQUIRE(merged_ab.findParameter(StyleParamKey::join).key == StyleParamKey::join);
        REQUIRE(merged_ab.findParameter(StyleParamKey::join).value.get<std::string>() == "value_4a");
        REQUIRE(merged_ab.findParameter(StyleParamKey::order).key == StyleParamKey::order);
        REQUIRE(merged_ab.findParameter(StyleParamKey::order).value.get<std::string>() == "value_0b");
        REQUIRE(merged_ab.findParameter(StyleParamKey::style).key == StyleParamKey::style);
        REQUIRE(merged_ab.findParameter(StyleParamKey::style).value.get<std::string>() == "value_4b");
        REQUIRE(merged_ab.findParameter(StyleParamKey::width).key == StyleParamKey::width);
        REQUIRE(merged_ab.findParameter(StyleParamKey::width).value.get<std::string>() == "value_2b");

        // explicit style wins
        REQUIRE(merged_ab.getStyleName() == "value_4b");
    }

    {
        DrawRuleMergeSet ruleSet;
        ruleSet.mergeRules(layer_b);
        ruleSet.mergeRules(layer_a);

        REQUIRE(ruleSet.matchedRules().size() == 1);

        auto& merged_ba = ruleSet.matchedRules()[0];

        REQUIRE(merged_ba.findParameter(StyleParamKey::cap).key == StyleParamKey::cap);
        REQUIRE(merged_ba.findParameter(StyleParamKey::cap).value.get<std::string>() == "value_3b");
        REQUIRE(merged_ba.findParameter(StyleParamKey::color).key == StyleParamKey::color);
        REQUIRE(merged_ba.findParameter(StyleParamKey::color).value.get<std::string>() == "value_1b");
        REQUIRE(merged_ba.findParameter(StyleParamKey::join).key == StyleParamKey::join);
        REQUIRE(merged_ba.findParameter(StyleParamKey::join).value.get<std::string>() == "value_4a");
        REQUIRE(merged_ba.findParameter(StyleParamKey::order).key == StyleParamKey::order);
        REQUIRE(merged_ba.findParameter(StyleParamKey::order).value.get<std::string>() == "value_0b");
        REQUIRE(merged_ba.findParameter(StyleParamKey::style).key == StyleParamKey::style);
        REQUIRE(merged_ba.findParameter(StyleParamKey::style).value.get<std::string>() == "value_4b");
        REQUIRE(merged_ba.findParameter(StyleParamKey::width).key == StyleParamKey::width);
        REQUIRE(merged_ba.findParameter(StyleParamKey::width).value.get<std::string>() == "value_2b");

        // explicit style wins
        REQUIRE(merged_ba.getStyleName() == "value_4b");
    }

    {
        DrawRuleMergeSet ruleSet;
        ruleSet.mergeRules(layer_c);
        ruleSet.mergeRules(layer_b);

        REQUIRE(ruleSet.matchedRules().size() == 1);

        auto& merged_bc = ruleSet.matchedRules()[0];

        // for (size_t i = 0; i < StyleParamKeySize; i++) {
        //     auto* param = merged_bc.params[i];
        //     if (!param) { continue; }
        //     REQUIRE(param->key == rule_b[0].parameters[i].key);
        //     REQUIRE(param->value.get<std::string>() ==
        //             rule_b[0].parameters[i].value.get<std::string>());
        // }

        // explicit style wins
        REQUIRE(merged_bc.getStyleName() == "value_4b");
    }

}

TEST_CASE("DrawRule locates and outputs a parameter that it contains", "[DrawRule]") {

    std::string str;

    const SceneLayer layer_a = { "a", Filter(), instance_a(), {} };
    const SceneLayer layer_b = { "b", Filter(), instance_b(), {} };

    DrawRuleMergeSet a;
    a.mergeRules(layer_a);
    auto& rule_a = a.matchedRules()[0];

    REQUIRE(rule_a.get(StyleParamKey::order, str)); REQUIRE(str == "value_0a");
    REQUIRE(rule_a.get(StyleParamKey::color, str)); REQUIRE(str == "value_1a");
    REQUIRE(rule_a.get(StyleParamKey::join, str)); REQUIRE(str == "value_4a");

    DrawRuleMergeSet b;
    b.mergeRules(layer_b);
    auto& rule_b = b.matchedRules()[0];

    REQUIRE(rule_b.get(StyleParamKey::color, str)); REQUIRE(str == "value_1b");
    REQUIRE(rule_b.get(StyleParamKey::width, str)); REQUIRE(str == "value_2b");
    REQUIRE(rule_b.get(StyleParamKey::cap, str)); REQUIRE(str == "value_3b");
    REQUIRE(rule_b.get(StyleParamKey::order, str)); REQUIRE(str == "value_0b");
}

TEST_CASE("DrawRule correctly reports that it doesn't contain a parameter", "[DrawRule]") {
    std::string str;

    const SceneLayer layer_a = { "a", Filter(), instance_a(), {} };
    DrawRuleMergeSet a;
    a.mergeRules(layer_a);
    REQUIRE(!a.matchedRules()[0].get(StyleParamKey::width, str)); REQUIRE(str == "");


    const SceneLayer layer_b = { "b", Filter(), instance_b(), {} };
    DrawRuleMergeSet b;
    b.mergeRules(layer_b);
    REQUIRE(!b.matchedRules()[0].get(StyleParamKey::join, str)); REQUIRE(str == "");

    const SceneLayer layer_c = { "c", Filter(), instance_c(), {} };
    DrawRuleMergeSet c;
    c.mergeRules(layer_c);
    REQUIRE(!c.matchedRules()[0].get(StyleParamKey::order, str)); REQUIRE(str == "");


}
