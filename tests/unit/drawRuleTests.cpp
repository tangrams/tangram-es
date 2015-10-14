#include "catch.hpp"

#include "scene/drawRule.h"
#include "scene/sceneLayer.h"

#include <cstdio>
#include <algorithm>

using namespace Tangram;

// Functions to initialize DrawRule instances
const int dg1 = 0;
const int dg2 = 1;

StaticDrawRule instance_a() {

    std::vector<StyleParam> params = {
        { StyleParamKey::order, "value_0a" },
        { StyleParamKey::join, "value_4a" },
        { StyleParamKey::color, "value_1a" }
    };

    return { "dg1", dg1, std::move(params) };

}

StaticDrawRule instance_b() {

    std::vector<StyleParam> params = {
        { StyleParamKey::order, "value_0b" },
        { StyleParamKey::width, "value_2b" },
        { StyleParamKey::color, "value_1b" },
        { StyleParamKey::cap, "value_3b" },
        { StyleParamKey::style, "value_4b" }
    };

    return { "dg1", dg1, std::move(params) };

}

StaticDrawRule instance_c() {

    std::vector<StyleParam> params = {};

    // changed from dg2 - styles will not be merged otherwise
    return { "dg1", dg1, params };

}

TEST_CASE("DrawRule correctly merges with another DrawRule", "[DrawRule]") {
    const std::vector<StaticDrawRule> rule_a{instance_a()};
    const std::vector<StaticDrawRule> rule_b{instance_b()};
    const std::vector<StaticDrawRule> rule_c{instance_c()};

    {
        Styling styling;
        styling.mergeRules(rule_a);

        REQUIRE(styling.styles.size() == 1);
        auto& merged_ab = styling.styles[0];

        for (int i = 0; i < StyleParamKeySize; i++) {
            auto* param = merged_ab.params[i];
            if (!param) {
                logMsg("param : none %d\n", i);
                continue;
            }
            logMsg("param : %s\n", param->toString().c_str());
        }

        styling.mergeRules(rule_b);

        REQUIRE(styling.styles.size() == 1);

        // printf("rule_a:\n %s", rule_a.toString().c_str());
        // printf("rule_c:\n %s", rule_c.toString().c_str());
        // printf("merged_ac:\n %s", merged_ac.toString().c_str());
        for (int i = 0; i < StyleParamKeySize; i++) {
            auto* param = merged_ab.params[i];
            if (!param) {
                logMsg("param : none %d\n", i);
                continue;
            }
            logMsg("param : %s\n", param->toString().c_str());
        }
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::cap)]->key == StyleParamKey::cap);
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::cap)]->value.get<std::string>() == "value_3b");
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::color)]->key == StyleParamKey::color);
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::color)]->value.get<std::string>() == "value_1b");
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::join)]->key == StyleParamKey::join);
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::join)]->value.get<std::string>() == "value_4a");
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::order)]->key == StyleParamKey::order);
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::order)]->value.get<std::string>() == "value_0b");
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::style)]->key == StyleParamKey::style);
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::style)]->value.get<std::string>() == "value_4b");
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::width)]->key == StyleParamKey::width);
        REQUIRE(merged_ab.params[static_cast<uint8_t>(StyleParamKey::width)]->value.get<std::string>() == "value_2b");

        // explicit style wins
        REQUIRE(merged_ab.getStyleName() == "value_4b");
    }

    {
        Styling styling;
        styling.mergeRules(rule_b);
        styling.mergeRules(rule_a);

        auto& merged_ba = styling.styles[0];

        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::cap)]->key == StyleParamKey::cap);
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::cap)]->value.get<std::string>() == "value_3b");
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::color)]->key == StyleParamKey::color);
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::color)]->value.get<std::string>() == "value_1a");
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::join)]->key == StyleParamKey::join);
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::join)]->value.get<std::string>() == "value_4a");
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::order)]->key == StyleParamKey::order);
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::order)]->value.get<std::string>() == "value_0a");
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::style)]->key == StyleParamKey::style);
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::style)]->value.get<std::string>() == "value_4b");
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::width)]->key == StyleParamKey::width);
        REQUIRE(merged_ba.params[static_cast<uint8_t>(StyleParamKey::width)]->value.get<std::string>() == "value_2b");

        // explicit style wins
        REQUIRE(merged_ba.getStyleName() == "value_4b");
    }

    {
        Styling styling;
        styling.mergeRules(rule_c);
        styling.mergeRules(rule_b);

        auto& merged_bc = styling.styles[0];

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

    const std::vector<StaticDrawRule> srule_a{instance_a()};
    const std::vector<StaticDrawRule> srule_b{instance_b()};

    Styling a;
    a.mergeRules(srule_a);
    auto& rule_a = a.styles[0];

    REQUIRE(rule_a.get(StyleParamKey::order, str)); REQUIRE(str == "value_0a");
    REQUIRE(rule_a.get(StyleParamKey::color, str)); REQUIRE(str == "value_1a");
    REQUIRE(rule_a.get(StyleParamKey::join, str)); REQUIRE(str == "value_4a");

    Styling b;
    b.mergeRules(srule_b);
    auto& rule_b = b.styles[0];

    REQUIRE(rule_b.get(StyleParamKey::color, str)); REQUIRE(str == "value_1b");
    REQUIRE(rule_b.get(StyleParamKey::width, str)); REQUIRE(str == "value_2b");
    REQUIRE(rule_b.get(StyleParamKey::cap, str)); REQUIRE(str == "value_3b");
    REQUIRE(rule_b.get(StyleParamKey::order, str)); REQUIRE(str == "value_0b");
}

TEST_CASE("DrawRule correctly reports that it doesn't contain a parameter", "[DrawRule]") {
    std::string str;

    const std::vector<StaticDrawRule> srule_a{instance_a()};
    Styling a;
    a.mergeRules(srule_a);
    REQUIRE(!a.styles[0].get(StyleParamKey::width, str)); REQUIRE(str == "");


    const std::vector<StaticDrawRule> srule_b{instance_b()};
    Styling b;
    b.mergeRules(srule_b);
    REQUIRE(!b.styles[0].get(StyleParamKey::join, str)); REQUIRE(str == "");

    const std::vector<StaticDrawRule> srule_c{instance_c()};
    Styling c;
    c.mergeRules(srule_c);
    REQUIRE(!c.styles[0].get(StyleParamKey::order, str)); REQUIRE(str == "");


}
