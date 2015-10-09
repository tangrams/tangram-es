#include "catch.hpp"

#include "scene/drawRule.h"
#include <cstdio>
#include <algorithm>

using namespace Tangram;

// Functions to initialize DrawRule instances
const int dg1 = 0;
const int dg2 = 1;

DrawRule instance_a() {

    std::vector<StyleParam> params = {
        { StyleParamKey::order, "value_0a" },
        { StyleParamKey::join, "value_4a" },
        { StyleParamKey::color, "value_1a" }
    };

    return { "dg1", dg1, params };

}

DrawRule instance_b() {

    std::vector<StyleParam> params = {
        { StyleParamKey::order, "value_0b" },
        { StyleParamKey::width, "value_2b" },
        { StyleParamKey::color, "value_1b" },
        { StyleParamKey::cap, "value_3b" },
        { StyleParamKey::style, "value_4b" }
    };

    return { "dg1", dg1, params };

}

DrawRule instance_c() {

    std::vector<StyleParam> params = {};

    return { "dg2", dg2, params };

}

TEST_CASE("DrawRule contains sorted list of parameters after construction", "[DrawRule]") {

    auto rule_a = instance_a();
    auto rule_b = instance_b();
    auto rule_c = instance_c();

    REQUIRE(std::is_sorted(rule_a.parameters.begin(), rule_a.parameters.end()));
    REQUIRE(std::is_sorted(rule_b.parameters.begin(), rule_b.parameters.end()));
    REQUIRE(std::is_sorted(rule_c.parameters.begin(), rule_c.parameters.end()));

}

TEST_CASE("DrawRule correctly merges with another DrawRule", "[DrawRule]") {

    {
        auto rule_a = instance_a();
        auto rule_b = instance_b();

        auto merged_ab = rule_b.merge(rule_a);

        // printf("rule_a:\n %s", rule_a.toString().c_str());
        // printf("rule_c:\n %s", rule_c.toString().c_str());
        // printf("merged_ac:\n %s", merged_ac.toString().c_str());

        REQUIRE(merged_ab.parameters[0].key == StyleParamKey::cap);
        REQUIRE(merged_ab.parameters[0].value.get<std::string>() == "value_3b");
        REQUIRE(merged_ab.parameters[1].key == StyleParamKey::color);
        REQUIRE(merged_ab.parameters[1].value.get<std::string>() == "value_1b");
        REQUIRE(merged_ab.parameters[2].key == StyleParamKey::join);
        REQUIRE(merged_ab.parameters[2].value.get<std::string>() == "value_4a");
        REQUIRE(merged_ab.parameters[3].key == StyleParamKey::order);
        REQUIRE(merged_ab.parameters[3].value.get<std::string>() == "value_0b");
        REQUIRE(merged_ab.parameters[4].key == StyleParamKey::style);
        REQUIRE(merged_ab.parameters[4].value.get<std::string>() == "value_4b");
        REQUIRE(merged_ab.parameters[5].key == StyleParamKey::width);
        REQUIRE(merged_ab.parameters[5].value.get<std::string>() == "value_2b");

        // explicit style wins
        REQUIRE(merged_ab.getStyleName() == "value_4b");
    }

    {
        auto rule_a = instance_a();
        auto rule_b = instance_b();

        auto merged_ba = rule_a.merge(rule_b);

        REQUIRE(merged_ba.parameters[0].key == StyleParamKey::cap);
        REQUIRE(merged_ba.parameters[0].value.get<std::string>() == "value_3b");
        REQUIRE(merged_ba.parameters[1].key == StyleParamKey::color);
        REQUIRE(merged_ba.parameters[1].value.get<std::string>() == "value_1a");
        REQUIRE(merged_ba.parameters[2].key == StyleParamKey::join);
        REQUIRE(merged_ba.parameters[2].value.get<std::string>() == "value_4a");
        REQUIRE(merged_ba.parameters[3].key == StyleParamKey::order);
        REQUIRE(merged_ba.parameters[3].value.get<std::string>() == "value_0a");
        REQUIRE(merged_ba.parameters[4].key == StyleParamKey::style);
        REQUIRE(merged_ba.parameters[4].value.get<std::string>() == "value_4b");
        REQUIRE(merged_ba.parameters[5].key == StyleParamKey::width);
        REQUIRE(merged_ba.parameters[5].value.get<std::string>() == "value_2b");

        // explicit style wins
        REQUIRE(merged_ba.getStyleName() == "value_4b");
    }

    {
        auto rule_b = instance_b();
        auto rule_c = instance_c();

        auto merged_bc = rule_b.merge(rule_c);

        for (size_t i = 0; i < merged_bc.parameters.size(); i++) {
            REQUIRE(merged_bc.parameters[i].key == rule_b.parameters[i].key);
            REQUIRE(merged_bc.parameters[i].value.get<std::string>() ==
                    rule_b.parameters[i].value.get<std::string>());
        }

        // explicit style wins
        REQUIRE(merged_bc.getStyleName() == "value_4b");
    }

}

TEST_CASE("DrawRule locates and outputs a parameter that it contains", "[DrawRule]") {

    auto rule_a = instance_a();
    auto rule_b = instance_b();
    auto rule_c = instance_c();

    std::string str;

    REQUIRE(rule_a.get(StyleParamKey::order, str)); REQUIRE(str == "value_0a");
    REQUIRE(rule_a.get(StyleParamKey::color, str)); REQUIRE(str == "value_1a");
    REQUIRE(rule_a.get(StyleParamKey::join, str)); REQUIRE(str == "value_4a");
    REQUIRE(rule_b.get(StyleParamKey::color, str)); REQUIRE(str == "value_1b");
    REQUIRE(rule_b.get(StyleParamKey::width, str)); REQUIRE(str == "value_2b");
    REQUIRE(rule_b.get(StyleParamKey::cap, str)); REQUIRE(str == "value_3b");
    REQUIRE(rule_b.get(StyleParamKey::order, str)); REQUIRE(str == "value_0b");

}

TEST_CASE("DrawRule correctly reports that it doesn't contain a parameter", "[DrawRule]") {

    auto rule_a = instance_a();
    auto rule_b = instance_b();
    auto rule_c = instance_c();

    std::string str;

    REQUIRE(!rule_a.get(StyleParamKey::width, str)); REQUIRE(str == "");
    REQUIRE(!rule_b.get(StyleParamKey::join, str)); REQUIRE(str == "");
    REQUIRE(!rule_c.get(StyleParamKey::order, str)); REQUIRE(str == "");
}
