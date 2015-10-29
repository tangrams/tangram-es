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

    StaticDrawRule rule = { "dg0", dg0, { { StyleParamKey::order, "value_a" } } };

    return { "layer_a", f, { rule }, {} };
}

SceneLayer instance_b() {

    Filter f = Filter::MatchAny({}); // passes nothing

    StaticDrawRule rule = { "dg1", dg1, { { StyleParamKey::order, "value_b" } } };

    return { "layer_b", f, { rule }, {} };
}

SceneLayer instance_c() {

    Filter f = Filter(); // passes everything

    StaticDrawRule rule = { "dg2", dg2, { { StyleParamKey::order, "value_c" } } };

    return { "layer_c", f, { rule }, { instance_a(), instance_b() } };
}

SceneLayer instance_d() {

    Filter f = Filter(); // passes everything

    StaticDrawRule rule = { "dg0", dg0, { { StyleParamKey::order, "value_d" } } };

    return { "layer_d", f, { rule }, {} };
}

SceneLayer instance_e() {

    Filter f = Filter(); // passes everything

    StaticDrawRule rule = { "dg2", dg2, { { StyleParamKey::order, "value_e" } } };

    return { "layer_e", f, { rule }, { instance_c(), instance_d() } };
}

SceneLayer instance_2() {

    Filter f = Filter::MatchExistence("two", true);

    StaticDrawRule rule = { "group2", group2, {} };

    return { "subLayer2", f, { rule }, {} };
}

SceneLayer instance_1() {

    Filter f = Filter::MatchExistence("one", true);

    StaticDrawRule rule = { "group1", group1, {} };

    return { "subLayer1", f, { rule }, {} };
}

SceneLayer instance() {

    Filter f = Filter::MatchExistence("base", true);

    StaticDrawRule rule = { "group1", group1, { {StyleParamKey::order, "a" } } };

    return { "layer", f, { rule }, { instance_1(), instance_2() } };
}
#if 0
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
        matches = layer.match(f1, ctx);

        REQUIRE(matches.size() == 1);
        REQUIRE(matches[0].getStyleName() == "group1");
    }

    {
        matches.clear();
        f2.props.add("one", "blah"); // Should match Base and subLayer1
        f2.props.add("base", "blah");
        matches = layer.match(f2, ctx);

        REQUIRE(matches.size() == 1);
        REQUIRE(matches[0].getStyleName() == "group1");
        REQUIRE(matches[0].parameters[0].key == StyleParamKey::order);
        REQUIRE(matches[0].parameters[0].value.get<std::string>() == "a");
    }

    {
        matches.clear();
        f3.props.add("two", "blah"); // Should not match anything as uber layer will not be satisfied
        matches = layer.match(f3, ctx);

        REQUIRE(matches.size() == 0);
    }

    {
        matches.clear();
        f4.props.add("two", "blah");
        f4.props.add("base", "blah"); // Should match Base and subLayer2
        matches = layer.match(f4, ctx);

        REQUIRE(matches.size() == 2);
        REQUIRE(matches[0].getStyleName() == "group1");
        REQUIRE(matches[0].parameters[0].key == StyleParamKey::order);
        REQUIRE(matches[0].parameters[0].value.get<std::string>() == "a");
        REQUIRE(matches[1].getStyleName() == "group2");
    }

}
#endif

TEST_CASE("SceneLayer matches correct rules for a feature and context", "[SceneLayer][Filter]") {

    Feature feat;
    Context ctx;

    {
        Styling styling;
        auto layer_a = instance_a();

        styling.match(feat, layer_a, ctx);
        auto& matches_a = styling.styles;

        REQUIRE(matches_a.size() == 1);
        REQUIRE(matches_a[0].getStyleName() == "dg0");
    }

    {
        Styling styling;
        auto layer_b = instance_b();

        styling.match(feat, layer_b, ctx);
        auto& matches_b = styling.styles;

        REQUIRE(matches_b.size() == 0);
    }

}

#if 0
TEST_CASE("SceneLayer matches correct sublayer rules for a feature and context", "[SceneLayer][Filter]") {

    Feature feat;
    Context ctx;
    Styling styling;

    auto layer_c = instance_c();

    std::vector<DrawRule> matches;
    layer_c.match(feat, ctx, styling);
    auto& matches = styling.styles;

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
    matches = layer_e.match(feat, ctx);

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
#endif
