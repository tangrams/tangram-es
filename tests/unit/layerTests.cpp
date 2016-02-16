#include "catch.hpp"

#include "scene/sceneLayer.h"
#include "data/tileData.h"
#include "scene/styleContext.h"

using namespace Tangram;

using Context = StyleContext;

// Functions to initialize SceneLayer instances
const int dg0 = 0;
const int dg1 = 1;
const int dg2 = 2;

const int group1 = 1;
const int group2 = 2;

SceneLayer instance_a() {

    Filter f = Filter(); // passes everything

    DrawRuleData rule = { "dg0", dg0, { { StyleParamKey::order, "value_a" } } };

    return { "layer_a", f, { rule }, {} };
}

SceneLayer instance_b() {

    Filter f = Filter::MatchAny({}); // passes nothing

    DrawRuleData rule = { "dg1", dg1, { { StyleParamKey::order, "value_b" } } };

    return { "layer_b", f, { rule }, {} };
}

SceneLayer instance_c() {

    Filter f = Filter(); // passes everything

    DrawRuleData rule = { "dg2", dg2, { { StyleParamKey::order, "value_c" } } };

    return { "layer_c", f, { rule }, { instance_a(), instance_b() } };
}

SceneLayer instance_d() {

    Filter f = Filter(); // passes everything

    DrawRuleData rule = { "dg0", dg0, { { StyleParamKey::order, "value_d" } } };

    return { "layer_d", f, { rule }, {} };
}

SceneLayer instance_e() {

    Filter f = Filter(); // passes everything

    DrawRuleData rule = { "dg2", dg2, { { StyleParamKey::order, "value_e" } } };

    return { "layer_e", f, { rule }, { instance_c(), instance_d() } };
}

SceneLayer instance_2() {

    Filter f = Filter::MatchExistence("two", true);

    DrawRuleData rule = { "group2", group2, {} };

    return { "subLayer2", f, { rule }, {} };
}

SceneLayer instance_1() {

    Filter f = Filter::MatchExistence("one", true);

    DrawRuleData rule = { "group1", group1, {} };

    return { "subLayer1", f, { rule }, {} };
}

SceneLayer instance() {

    Filter f = Filter::MatchExistence("base", true);

    DrawRuleData rule = { "group1", group1, { {StyleParamKey::order, "a" } } };

    return { "layer", f, { rule }, { instance_1(), instance_2() } };
}

TEST_CASE("SceneLayer", "[SceneLayer][Filter][DrawRule][Match][Merge]") {

    Feature f1;
    Feature f2;
    Feature f3;
    Feature f4;
    Context ctx;

    auto layer = instance();

    {
        DrawRuleMergeSet ruleSet;
        f1.props.set("base", "blah"); // Should match Base Layer
        ruleSet.match(f1, layer, ctx);
        auto& matches = ruleSet.matchedRules();

        REQUIRE(matches.size() == 1);
        REQUIRE(matches[0].getStyleName() == "group1");
    }

    {
        DrawRuleMergeSet ruleSet;
        f2.props.set("one", "blah"); // Should match Base and subLayer1
        f2.props.set("base", "blah");
        ruleSet.match(f2, layer, ctx);
        auto& matches = ruleSet.matchedRules();

        REQUIRE(matches.size() == 1);
        REQUIRE(matches[0].getStyleName() == "group1");
        REQUIRE(matches[0].findParameter(StyleParamKey::order).key == StyleParamKey::order);
        REQUIRE(matches[0].findParameter(StyleParamKey::order).value.get<std::string>() == "a");
    }

    {
        DrawRuleMergeSet ruleSet;
        f3.props.set("two", "blah"); // Should not match anything as uber layer will not be satisfied
        ruleSet.match(f3, layer, ctx);
        auto& matches = ruleSet.matchedRules();

        REQUIRE(matches.size() == 0);
    }

    {
        DrawRuleMergeSet ruleSet;
        f4.props.set("two", "blah");
        f4.props.set("base", "blah"); // Should match Base and subLayer2
        ruleSet.match(f4, layer, ctx);
        auto& matches = ruleSet.matchedRules();

        REQUIRE(matches.size() == 2);
        REQUIRE(matches[0].getStyleName() == "group1");
        REQUIRE(matches[0].findParameter(StyleParamKey::order).key == StyleParamKey::order);
        REQUIRE(matches[0].findParameter(StyleParamKey::order).value.get<std::string>() == "a");
        REQUIRE(matches[1].getStyleName() == "group2");
    }

}

TEST_CASE("SceneLayer matches correct rules for a feature and context", "[SceneLayer][Filter]") {

    Feature feat;
    Context ctx;

    {
        DrawRuleMergeSet ruleSet;
        auto layer_a = instance_a();

        ruleSet.match(feat, layer_a, ctx);
        auto& matches_a = ruleSet.matchedRules();

        REQUIRE(matches_a.size() == 1);
        REQUIRE(matches_a[0].getStyleName() == "dg0");
    }

    {
        DrawRuleMergeSet ruleSet;
        auto layer_b = instance_b();

        ruleSet.match(feat, layer_b, ctx);
        auto& matches_b = ruleSet.matchedRules();

        REQUIRE(matches_b.size() == 0);
    }

}

TEST_CASE("SceneLayer matches correct sublayer rules for a feature and context", "[SceneLayer][Filter]") {

    Feature feat;
    Context ctx;
    DrawRuleMergeSet ruleSet;

    auto layer_c = instance_c();

    ruleSet.match(feat, layer_c, ctx);
    auto& matches = ruleSet.matchedRules();

    REQUIRE(matches.size() == 2);

    REQUIRE(matches[0].getStyleName() == "dg2");
    REQUIRE(matches[1].getStyleName() == "dg0");

}

TEST_CASE("SceneLayer correctly merges rules matched from sublayer", "[SceneLayer][Filter]") {

    Feature feat;
    Context ctx;
    DrawRuleMergeSet ruleSet;

    auto layer_e = instance_e();

    ruleSet.match(feat, layer_e, ctx);
    auto& matches = ruleSet.matchedRules();

    REQUIRE(matches.size() == 2);

    // deeper match from layer_a should override parameters in same style from layer_d
    REQUIRE(matches[1].getStyleName() == "dg0");
    REQUIRE(matches[1].findParameter(StyleParamKey::order).key == StyleParamKey::order);
    REQUIRE(matches[1].findParameter(StyleParamKey::order).value.get<std::string>() == "value_a");

    // deeper match from layer_c should override parameters in same style from layer_e
    REQUIRE(matches[0].getStyleName() == "dg2");
    REQUIRE(matches[0].findParameter(StyleParamKey::order).key == StyleParamKey::order);
    REQUIRE(matches[0].findParameter(StyleParamKey::order).value.get<std::string>() == "value_c");

}
