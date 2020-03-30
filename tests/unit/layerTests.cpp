#include "catch.hpp"

#include "scene/sceneLayer.h"
#include "data/tileData.h"
#include "scene/styleContext.h"

using namespace Tangram;

namespace {

#define TAGS "[SceneLayer][Filter][DrawRule]"

TEST_CASE("SceneLayer feature matching", TAGS) {
    // layer_e:
    //   draw_group_2:
    //     order: order_e
    //   layer_d:
    //     draw_group_0:
    //       order: order_d
    //   layer_c:
    //     draw_group_2:
    //       order: order_c
    //     layer_b:
    //       filter:
    //         any: []
    //       draw_group_1:
    //         order: order_b
    //     layer_a:
    //       draw_group_0:
    //         order: order_a

    const Filter matchEverything;
    const Filter matchNothing = Filter::MatchAny({});

    const DrawRuleData ruleA = {"draw_group_0", 0, {{StyleParamKey::order, "order_a"}}};
    const SceneLayer layerA = {"layer_a", matchEverything, {ruleA}, {}, SceneLayer::Options()};

    const DrawRuleData ruleB = {"draw_group_1", 1, {{StyleParamKey::order, "order_b"}}};
    const SceneLayer layerB = {"layer_b", matchNothing, {ruleB}, {}, SceneLayer::Options()};

    const DrawRuleData ruleC = {"draw_group_2", 2, {{StyleParamKey::order, "order_c"}}};
    const SceneLayer layerC = {"layer_c", matchEverything, {ruleC}, {layerA, layerB}, SceneLayer::Options()};

    const DrawRuleData ruleD = {"draw_group_0", 0, {{StyleParamKey::order, "order_d"}}};
    const SceneLayer layerD = {"layer_d", matchEverything, {ruleD}, {}, SceneLayer::Options()};

    const DrawRuleData ruleE = {"draw_group_2", 2, {{StyleParamKey::order, "order_e"}}};
    const SceneLayer layerE = {"layer_e", matchEverything, {ruleE}, {layerC, layerD}, SceneLayer::Options()};

    Feature feature;
    StyleContext context;
    DrawRuleMergeSet ruleSet;

    SECTION("feature matches one draw group") {
        ruleSet.match(feature, layerA, context);
        auto& matches = ruleSet.matchedRules();

        REQUIRE(matches.size() == 1);
        REQUIRE(*matches[0].name == "draw_group_0");
    }

    SECTION("feature matches no draw groups") {
        ruleSet.match(feature, layerB, context);
        auto& matches = ruleSet.matchedRules();

        REQUIRE(matches.size() == 0);
    }

    SECTION("feature matches multiple draw groups") {
        ruleSet.match(feature, layerC, context);
        auto& matches = ruleSet.matchedRules();

        REQUIRE(matches.size() == 2);
        REQUIRE(*matches[0].name == "draw_group_2");
        REQUIRE(*matches[1].name == "draw_group_0");
    }

    SECTION("feature matches draw groups in multiple layers") {
        ruleSet.match(feature, layerE, context);
        auto& matches = ruleSet.matchedRules();

        REQUIRE(matches.size() == 2);

        // deeper match from layer_c should override parameters in same style from layer_e
        REQUIRE(*matches[0].name == "draw_group_2");
        REQUIRE(matches[0].hasParameterSet(StyleParamKey::order));
        REQUIRE(matches[0].findParameter(StyleParamKey::order).value.get<std::string>() == "order_c");

        // deeper match from layer_a should override parameters in same style from layer_d
        REQUIRE(*matches[1].name == "draw_group_0");
        REQUIRE(matches[1].hasParameterSet(StyleParamKey::order));
        REQUIRE(matches[1].findParameter(StyleParamKey::order).value.get<std::string>() == "order_a");
    }

}

TEST_CASE("SceneLayer exclusive and priority", TAGS) {
    // layer_j:
    // layer_i:
    //   exclusive: true
    // layer_h:
    //   priority: 2
    // layer_g:
    //   priority: 1
    // layer_f:
    //   exclusive: true
    //   priority: 3

    const Filter matchEverything;

    SceneLayer::Options optionsF;
    optionsF.exclusive = true;
    optionsF.priority = 3;
    const SceneLayer layerF = {"layer_f", matchEverything, {}, {}, optionsF};

    SceneLayer::Options optionsG;
    optionsG.priority = 1;
    const SceneLayer layerG = {"layer_g", matchEverything, {}, {}, optionsG};

    SceneLayer::Options optionsH;
    optionsH.priority = 2;
    const SceneLayer layerH = {"layer_h", matchEverything, {}, {}, optionsH};

    SceneLayer::Options optionsI;
    optionsI.exclusive = true;
    const SceneLayer layerI = {"layer_i", matchEverything, {}, {}, optionsI};

    SceneLayer::Options optionsJ;
    const SceneLayer layerJ = {"layer_j", matchEverything, {}, {}, optionsJ};

    Feature feature;
    StyleContext context;
    DrawRuleMergeSet ruleSet;

    SECTION("sort sublayers according to exclusive and priority") {
        const SceneLayer layer = {"layer", matchEverything, {}, {layerF, layerG, layerH, layerI, layerJ}, SceneLayer::Options()};

        REQUIRE(layer.sublayers().size() == 5);
        CHECK(layer.sublayers()[0].name() == "layer_f");
        CHECK(layer.sublayers()[1].name() == "layer_i");
        CHECK(layer.sublayers()[2].name() == "layer_g");
        CHECK(layer.sublayers()[3].name() == "layer_h");
        CHECK(layer.sublayers()[4].name() == "layer_j");
    }
}

} // namespace
