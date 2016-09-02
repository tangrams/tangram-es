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

    std::vector<StyleParam> params;
    params.emplace_back(StyleParamKey::order, std::string{"value_a"});

    std::vector<DrawRuleData> rules;
    rules.emplace_back("dg0", dg0, std::move(params));

    return { "layer_a", f, std::move(rules), {} };
}

SceneLayer instance_b() {

    Filter f = Filter::MatchAny({}); // passes nothing

    std::vector<StyleParam> params;
    params.emplace_back(StyleParamKey::order, std::string{"value_b"});

    std::vector<DrawRuleData> rules;
    rules.emplace_back("dg1", dg1, std::move(params));

    return { "layer_b", f, std::move(rules), {} };
}

SceneLayer instance_c() {

    Filter f = Filter(); // passes everything
    std::vector<StyleParam> params;
    params.emplace_back(StyleParamKey::order, std::string{"value_c"});

    std::vector<DrawRuleData> rules;
    rules.emplace_back("dg2", dg2, std::move(params));

    std::vector<SceneLayer> sublayers;
    sublayers.emplace_back(instance_a());
    sublayers.emplace_back(instance_b());

    return { "layer_c", f, std::move(rules), std::move(sublayers) };
}

SceneLayer instance_d() {

    Filter f = Filter(); // passes everything

    std::vector<StyleParam> params;
    params.emplace_back(StyleParamKey::order, std::string{"value_d"});

    std::vector<DrawRuleData> rules;
    rules.emplace_back("dg0", dg0, std::move(params));

    return { "layer_d", f, std::move(rules), {} };
}

SceneLayer instance_e() {

    Filter f = Filter(); // passes everything

    std::vector<StyleParam> params;
    params.emplace_back(StyleParamKey::order, std::string{"value_e"});

    std::vector<DrawRuleData> rules;
    rules.emplace_back("dg2", dg2, std::move(params));

    std::vector<SceneLayer> sublayers;
    sublayers.emplace_back(instance_c());
    sublayers.emplace_back(instance_d());

    return { "layer_e", f, std::move(rules), std::move(sublayers) };
}

SceneLayer instance_2() {

    Filter f = Filter::MatchExistence("two", true);

    std::vector<StyleParam> params;
    std::vector<DrawRuleData> rules;
    rules.emplace_back("group2", group2, std::move(params));

    return { "subLayer2", f, std::move(rules), {} };
}

SceneLayer instance_1() {

    Filter f = Filter::MatchExistence("one", true);

    std::vector<StyleParam> params;
    std::vector<DrawRuleData> rules;
    rules.emplace_back("group1", group1, std::move(params));

    return { "subLayer1", f, std::move(rules), {} };
}

SceneLayer instance() {

    Filter f = Filter::MatchExistence("base", true);

    std::vector<StyleParam> params;
    params.emplace_back(StyleParamKey::order, std::string{"a"});

    std::vector<DrawRuleData> rules;
    rules.emplace_back("group1", group1, std::move(params));

    std::vector<SceneLayer> sublayers;
    sublayers.emplace_back(instance_1());
    sublayers.emplace_back(instance_2());

    return { "layer", f, std::move(rules), std::move(sublayers) };
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
