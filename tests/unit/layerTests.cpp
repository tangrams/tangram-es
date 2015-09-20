#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "scene/sceneLayer.h"

using namespace Tangram;

using Context = StyleContext;

// Functions to initialize SceneLayer instances

SceneLayer instance_a() {

    Filter f = Filter(); // passes everything

    DrawRule rule = { "style_0", { { StyleParamKey::order, "value_a" } } };

    return { "layer_a", f, { rule }, {} };
}

SceneLayer instance_b() {

    Filter f = Filter::MatchAny({}); // passes nothing

    DrawRule rule = { "style_1", { { StyleParamKey::order, "value_b" } } };

    return { "layer_b", f, { rule }, {} };
}

SceneLayer instance_c() {

    Filter f = Filter(); // passes everything

    DrawRule rule = { "style_2", { { StyleParamKey::order, "value_c" } } };

    return { "layer_c", f, { rule }, { instance_a(), instance_b() } };
}

SceneLayer instance_d() {

    Filter f = Filter(); // passes everything

    DrawRule rule = { "style_0", { { StyleParamKey::order, "value_d" } } };

    return { "layer_d", f, { rule }, {} };
}

SceneLayer instance_e() {

    Filter f = Filter(); // passes everything

    DrawRule rule = { "style_2", { { StyleParamKey::order, "value_e" } } };

    return { "layer_e", f, { rule }, { instance_c(), instance_d() } };
}

TEST_CASE("SceneLayer matches correct rules for a feature and context", "[SceneLayer][Filter]") {

    Feature feat;
    Context ctx;

    auto layer_a = instance_a();

    std::vector<DrawRule> matches_a;
    layer_a.match(feat, ctx, matches_a);

    REQUIRE(matches_a.size() == 1);
    REQUIRE(matches_a[0].style == "style_0");

    auto layer_b = instance_b();

    std::vector<DrawRule> matches_b;
    layer_b.match(feat, ctx, matches_b);

    REQUIRE(matches_b.size() == 0);

}

TEST_CASE("SceneLayer matches correct sublayer rules for a feature and context", "[SceneLayer][Filter]") {

    Feature feat;
    Context ctx;

    auto layer_c = instance_c();

    std::vector<DrawRule> matches;
    layer_c.match(feat, ctx, matches);

    REQUIRE(matches.size() == 2);

    // matches should be in lexicographic order by style
    REQUIRE(matches[0].style == "style_0");
    REQUIRE(matches[1].style == "style_2");

}

TEST_CASE("SceneLayer correctly merges rules matched from sublayer", "[SceneLayer][Filter]") {

    Feature feat;
    Context ctx;

    auto layer_e = instance_e();

    std::vector<DrawRule> matches;
    layer_e.match(feat, ctx, matches);

    REQUIRE(matches.size() == 2);

    // deeper match from layer_a should override parameters in same style from layer_d
    REQUIRE(matches[0].style == "style_0");
    REQUIRE(matches[0].parameters[0].key == StyleParamKey::order);
    REQUIRE(matches[0].parameters[0].value.get<std::string>() == "value_a");

    // deeper match from layer_c should override parameters in same style from layer_e
    REQUIRE(matches[1].style == "style_2");
    REQUIRE(matches[1].parameters[0].key == StyleParamKey::order);
    REQUIRE(matches[1].parameters[0].value.get<std::string>() == "value_c");

}
