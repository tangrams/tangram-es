#include "catch.hpp"

#include "scene/drawRule.h"
#include "scene/sceneLayer.h"
#include "platform.h"

#include <cstdio>
#include <algorithm>

using namespace Tangram;
namespace {

#define TAGS "[DrawRule]"

void logDrawRule(const DrawRule& drawRule) {
    for (size_t i = 0; i < StyleParamKeySize; i++) {
        if (!drawRule.active[i]) {
            continue;
        }
        auto* param = drawRule.params[i].param;
        if (!param) {
            logMsg("param : none %d\n", i);
            continue;
        }
        logMsg("param : %s\n", param->toString().c_str());
    }
}

TEST_CASE("DrawRule correctly merges with another DrawRule", TAGS) {

    const Filter matchEverything;

    const DrawRuleData rule_a = { "draw_group_0", 0, {
            { StyleParamKey::order, "order_a" },
            { StyleParamKey::join, "join_a" }
    } };

    const DrawRuleData rule_b = { "draw_group_0", 0, {
            { StyleParamKey::order, "order_b" },
            { StyleParamKey::color, "color_b" },
            { StyleParamKey::style, "style_b" }
    } };

    const DrawRuleData rule_c = { "draw_group_1", 1, {} };

    const SceneLayer layer_a = { "a", matchEverything, { rule_a }, {}, SceneLayer::Options() };
    const SceneLayer layer_b = { "b", matchEverything, { rule_b }, {}, SceneLayer::Options() };
    const SceneLayer layer_c = { "c", matchEverything, { rule_c }, {}, SceneLayer::Options() };

    SECTION("correctly reports that it doesn't contain a parameter") {
        std::string str;
        DrawRuleMergeSet mergeSet;
        mergeSet.mergeRules(layer_a);
        auto& mergedRule = mergeSet.matchedRules()[0];
        REQUIRE(!mergedRule.get(StyleParamKey::width, str));
        REQUIRE(str.empty());
    }

    SECTION("locates and outputs a parameter that it does contain") {
        std::string str;
        DrawRuleMergeSet mergeSet;
        mergeSet.mergeRules(layer_a);
        auto& mergedRule = mergeSet.matchedRules()[0];

        REQUIRE(mergedRule.get(StyleParamKey::order, str));
        REQUIRE(str == "order_a");

        REQUIRE(mergedRule.get(StyleParamKey::join, str));
        REQUIRE(str == "join_a");
    }

    SECTION("matched rules at the same depth use value from the first merged layer") {
        {
            DrawRuleMergeSet mergeSet;
            mergeSet.mergeRules(layer_a);
            mergeSet.mergeRules(layer_b);

            REQUIRE(mergeSet.matchedRules().size() == 1);

            // logDrawRule(ruleSet.matchedRules()[0]);

            auto& mergedRule = mergeSet.matchedRules()[0];

            CHECK(mergedRule.findParameter(StyleParamKey::color).key == StyleParamKey::color);
            CHECK(mergedRule.findParameter(StyleParamKey::color).value.get<std::string>() == "color_b");
            CHECK(mergedRule.findParameter(StyleParamKey::join).key == StyleParamKey::join);
            CHECK(mergedRule.findParameter(StyleParamKey::join).value.get<std::string>() == "join_a");
            CHECK(mergedRule.findParameter(StyleParamKey::order).key == StyleParamKey::order);
            CHECK(mergedRule.findParameter(StyleParamKey::order).value.get<std::string>() == "order_a");
            CHECK(mergedRule.findParameter(StyleParamKey::style).key == StyleParamKey::style);
            CHECK(mergedRule.findParameter(StyleParamKey::style).value.get<std::string>() == "style_b");

            // explicit style wins
            CHECK(mergedRule.getStyleName() == "style_b");
        }

        {
            DrawRuleMergeSet mergeSet;
            mergeSet.mergeRules(layer_b);
            mergeSet.mergeRules(layer_a);

            REQUIRE(mergeSet.matchedRules().size() == 1);

            auto& mergedRule = mergeSet.matchedRules()[0];

            CHECK(mergedRule.findParameter(StyleParamKey::color).key == StyleParamKey::color);
            CHECK(mergedRule.findParameter(StyleParamKey::color).value.get<std::string>() == "color_b");
            CHECK(mergedRule.findParameter(StyleParamKey::join).key == StyleParamKey::join);
            CHECK(mergedRule.findParameter(StyleParamKey::join).value.get<std::string>() == "join_a");
            CHECK(mergedRule.findParameter(StyleParamKey::order).key == StyleParamKey::order);
            CHECK(mergedRule.findParameter(StyleParamKey::order).value.get<std::string>() == "order_b");
            CHECK(mergedRule.findParameter(StyleParamKey::style).key == StyleParamKey::style);
            CHECK(mergedRule.findParameter(StyleParamKey::style).value.get<std::string>() == "style_b");

            // explicit style wins
            CHECK(mergedRule.getStyleName() == "style_b");
        }
    }

    SECTION("draw rules with different names are merged separately"){
        DrawRuleMergeSet mergeSet;
        mergeSet.mergeRules(layer_b);
        mergeSet.mergeRules(layer_c);

        REQUIRE(mergeSet.matchedRules().size() == 2);

        auto& mergedRule0 = mergeSet.matchedRules()[0];
        auto& mergedRule1 = mergeSet.matchedRules()[1];

        CHECK(*mergedRule0.name == "draw_group_0");
        CHECK(*mergedRule1.name == "draw_group_1");
    }
}

}
