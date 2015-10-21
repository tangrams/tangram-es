#include "catch.hpp"

#include "scene/sceneLayer.h"
#include "data/tileData.h"
#include "scene/styleContext.h"

using namespace Tangram;

using Context = StyleContext;

// Functions to initialize SceneLayer instances

SceneLayer instance_a() {

    Filter f = Filter(); // passes everything

    DrawRule rule = { "dg0", { { StyleParamKey::order, "value_a" } } };

    return { "layer_a", f, { rule }, {} };
}

SceneLayer instance_b() {

    Filter f = Filter::MatchAny({}); // passes nothing

    DrawRule rule = { "dg1", { { StyleParamKey::order, "value_b" } } };

    return { "layer_b", f, { rule }, {} };
}

SceneLayer instance_c() {

    Filter f = Filter(); // passes everything

    DrawRule rule = { "dg2", { { StyleParamKey::order, "value_c" } } };

    return { "layer_c", f, { rule }, { instance_a(), instance_b() } };
}

SceneLayer instance_d() {

    Filter f = Filter(); // passes everything

    DrawRule rule = { "dg0", { { StyleParamKey::order, "value_d" } } };

    return { "layer_d", f, { rule }, {} };
}

SceneLayer instance_e() {

    Filter f = Filter(); // passes everything

    DrawRule rule = { "dg2", { { StyleParamKey::order, "value_e" } } };

    return { "layer_e", f, { rule }, { instance_c(), instance_d() } };
}

SceneLayer instance_2() {

    Filter f = Filter::MatchExistence("two", true);

    DrawRule rule = { "group2", {} };

    return { "subLayer2", f, { rule }, {} };
}

SceneLayer instance_1() {

    Filter f = Filter::MatchExistence("one", true);

    DrawRule rule = { "group1", {} };

    return { "subLayer1", f, { rule }, {} };
}

SceneLayer instance() {

    Filter f = Filter::MatchExistence("base", true);

    DrawRule rule = { "group1", { {StyleParamKey::order, "a" } } };

    return { "layer", f, { rule }, { instance_1(), instance_2() } };
}

TEST_CASE("SceneLayer", "[SceneLayer][Filter][DrawRule][Match][Merge]") {

    Feature f1;
    Feature f2;
    Feature f3;
    Feature f4;
    Context ctx;

    auto layer = instance();
    std::vector<DrawRule> matches;

    {
        f1.props.add("base", "blah"); // Should match Base Layer
        layer.match(f1, ctx, matches);

        REQUIRE(matches.size() == 1);
        REQUIRE(matches[0].getStyleName() == "group1");
    }

    {
        matches.clear();
        f2.props.add("one", "blah"); // Should match Base and subLayer1
        f2.props.add("base", "blah");
        layer.match(f2, ctx, matches);

        REQUIRE(matches.size() == 1);
        REQUIRE(matches[0].getStyleName() == "group1");
        REQUIRE(matches[0].parameters[0].key == StyleParamKey::order);
        REQUIRE(matches[0].parameters[0].value.get<std::string>() == "a");
    }

    {
        matches.clear();
        f3.props.add("two", "blah"); // Should not match anything as uber layer will not be satisfied
        layer.match(f3, ctx, matches);

        REQUIRE(matches.size() == 0);
    }

    {
        matches.clear();
        f4.props.add("two", "blah");
        f4.props.add("base", "blah"); // Should match Base and subLayer2
        layer.match(f4, ctx, matches);

        REQUIRE(matches.size() == 2);
        REQUIRE(matches[0].getStyleName() == "group1");
        REQUIRE(matches[0].parameters[0].key == StyleParamKey::order);
        REQUIRE(matches[0].parameters[0].value.get<std::string>() == "a");
        REQUIRE(matches[1].getStyleName() == "group2");
    }

}

TEST_CASE("SceneLayer matches correct rules for a feature and context", "[SceneLayer][Filter]") {

    Feature feat;
    Context ctx;

    auto layer_a = instance_a();

    std::vector<DrawRule> matches_a;
    layer_a.match(feat, ctx, matches_a);

    REQUIRE(matches_a.size() == 1);
    REQUIRE(matches_a[0].getStyleName() == "dg0");

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

    // matches should be in lexicographic order by name
    REQUIRE(matches[0].getStyleName() == "dg0");
    REQUIRE(matches[1].getStyleName() == "dg2");

}

TEST_CASE("SceneLayer correctly merges rules matched from sublayer", "[SceneLayer][Filter]") {

    Feature feat;
    Context ctx;

    auto layer_e = instance_e();

    std::vector<DrawRule> matches;
    layer_e.match(feat, ctx, matches);

    REQUIRE(matches.size() == 2);

    // deeper match from layer_a should override parameters in same style from layer_d
    REQUIRE(matches[0].getStyleName() == "dg0");
    REQUIRE(matches[0].parameters[0].key == StyleParamKey::order);
    REQUIRE(matches[0].parameters[0].value.get<std::string>() == "value_a");

    // deeper match from layer_c should override parameters in same style from layer_e
    REQUIRE(matches[1].getStyleName() == "dg2");
    REQUIRE(matches[1].parameters[0].key == StyleParamKey::order);
    REQUIRE(matches[1].parameters[0].value.get<std::string>() == "value_c");

}
