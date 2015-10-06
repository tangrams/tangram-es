#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "data/filters.h"
#include "yaml-cpp/yaml.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"
#include "util/builders.h"
#include "platform.h"

using namespace Tangram;

TEST_CASE( "", "[Duktape][init]") {
    StyleContext();
}

TEST_CASE( "Test filter without feature being set", "[Duktape][evalFilterFn]") {
    StyleContext ctx;
    ctx.addFunction("fn", R"(function() { return feature.name === undefined; })");
    ctx.addAccessor("name");
    REQUIRE(ctx.evalFilterFn("fn") == true);

    ctx.addFunction("fn2", R"(function() { return feature.name === ''; })");
    REQUIRE(ctx.evalFilterFn("fn2") == false);
}

TEST_CASE( "Test evalFilterFn with feature", "[Duktape][evalFilterFn]") {
    Feature feature;
    feature.props.add("a", "A");
    feature.props.add("b", "B");
    feature.props.add("n", 42);

    StyleContext ctx;
    ctx.setFeature(feature);

    ctx.addFunction("fn_a", R"(function() { return feature.a === 'A' })");
    REQUIRE(ctx.evalFilterFn("fn_a") == true);

    ctx.addFunction("fn_b", R"(function() { return feature.b === 'B' })");
    REQUIRE(ctx.evalFilterFn("fn_b") == true);

    ctx.addFunction("fn_n", R"(function() { return feature.n === 42 })");
    REQUIRE(ctx.evalFilterFn("fn_n") == true);

    ctx.addFunction("fn_n2", R"(function() { return feature.n === 43 })");
    REQUIRE(ctx.evalFilterFn("fn_n2") == false);

    ctx.addFunction("fn_n3", R"(function() { return feature.n === '42' })");
    REQUIRE(ctx.evalFilterFn("fn_n3") == false);
}

TEST_CASE( "Test evalFilterFn with feature and globals", "[Duktape][evalFilterFn]") {
    Feature feature;
    feature.props.add("scalerank", 2);

    StyleContext ctx;
    ctx.setFeature(feature);
    ctx.setGlobal("$zoom", 5);

    ctx.addFunction("fn", R"(function() { return (feature.scalerank * .5) <= ($zoom - 4); })");
    REQUIRE(ctx.evalFilterFn("fn") == true);

    ctx.setGlobal("$zoom", 4);
    REQUIRE(ctx.evalFilterFn("fn") == false);
}

TEST_CASE( "Test evalFilterFn with different features", "[Duktape][evalFilterFn]") {
    StyleContext ctx;

    ctx.addFunction("fn", R"(function() { return feature.scalerank === 2; })");

    Feature feat1;
    feat1.props.add("scalerank", 2);

    ctx.setFeature(feat1);
    REQUIRE(ctx.evalFilterFn("fn") == true);

    Feature feat2;
    ctx.setFeature(feat2);
    REQUIRE(ctx.evalFilterFn("fn") == false);

    ctx.setFeature(feat1);
    REQUIRE(ctx.evalFilterFn("fn") == true);
}

TEST_CASE( "Test numeric global", "[Duktape][setGlobal]") {
    StyleContext ctx;
    ctx.setGlobal("$zoom", 10);
    ctx.addFunction("fn", R"(function() { return $zoom === 10 })");

    REQUIRE(ctx.evalFilterFn("fn") == true);

    ctx.setGlobal("$zoom", 0);

    REQUIRE(ctx.evalFilterFn("fn") == false);
}

TEST_CASE( "Test string global", "[Duktape][setGlobal]") {
    StyleContext ctx;
    ctx.setGlobal("$layer", "test");
    ctx.addFunction("fn", R"(function() { return $layer === 'test' })");

    REQUIRE(ctx.evalFilterFn("fn") == true);

    ctx.setGlobal("$layer", "none");

    REQUIRE(ctx.evalFilterFn("fn") == false);
}

TEST_CASE( "Test evalStyleFn - StyleParamKey::order", "[Duktape][evalStyleFn]") {
    Feature feat;
    feat.props.add("sort_key", 2);

    StyleContext ctx;
    ctx.setFeature(feat);
    ctx.addFunction("fn", R"(function () { return feature.sort_key + 5 })");

    StyleParam::Value value;

    REQUIRE(ctx.evalStyleFn("fn", StyleParamKey::order, value) == true);
    REQUIRE(value.is<uint32_t>() == true);
    REQUIRE(value.get<uint32_t>() == 7);
}

TEST_CASE( "Test evalStyleFn - StyleParamKey::color", "[Duktape][evalStyleFn]") {

    StyleContext ctx;
    StyleParam::Value value;

    ctx.addFunction("fn_s", R"(function () { return '#f0f'; })");
    REQUIRE(ctx.evalStyleFn("fn_s", StyleParamKey::color, value) == true);
    REQUIRE(value.is<uint32_t>() == true);
    REQUIRE(value.get<uint32_t>() == 0xffff00ff);

    ctx.addFunction("fn_i", R"(function () { return 0xff00ffff; })");
    REQUIRE(ctx.evalStyleFn("fn_i", StyleParamKey::color, value) == true);
    REQUIRE(value.is<uint32_t>() == true);
    REQUIRE(value.get<uint32_t>() == 0xff00ffff);

    ctx.addFunction("fn_a", R"(function () { return [1.0, 1.0, 0.0, 1.0] })");
    REQUIRE(ctx.evalStyleFn("fn_a", StyleParamKey::color, value) == true);
    REQUIRE(value.is<uint32_t>() == true);
    REQUIRE(value.get<uint32_t>() == 0xffffff00);

    ctx.addFunction("fn_a2", R"(function () { return [0.0, 1.0, 0.0] })");
    REQUIRE(ctx.evalStyleFn("fn_a2", StyleParamKey::color, value) == true);
    REQUIRE(value.is<uint32_t>() == true);
    REQUIRE(value.get<uint32_t>() == 0xff00ff00);

}

TEST_CASE( "Test evalStyleFn - StyleParamKey::width", "[Duktape][evalStyleFn]") {
    Feature feat;
    feat.props.add("width", 2.0);

    StyleContext ctx;
    ctx.setFeature(feat);
    ctx.addFunction("fn", R"(function () { return feature.width * 2.3; })");

    StyleParam::Value value;

    REQUIRE(ctx.evalStyleFn("fn", StyleParamKey::width, value) == true);
    REQUIRE(value.is<StyleParam::Width>() == true);
    REQUIRE(value.get<StyleParam::Width>().value == 4.6f);
}

TEST_CASE( "Test evalStyleFn - StyleParamKey::extrude", "[Duktape][evalStyleFn]") {
    Feature feat;
    feat.props.add("width", 2.0);

    StyleContext ctx;
    ctx.setFeature(feat);
    ctx.addFunction("fn_t", R"(function () { return true; })");
    ctx.addFunction("fn_f", R"(function () { return false; })");
    ctx.addFunction("fn_a", R"(function () { return [1.1, 2.2]; })");

    StyleParam::Value value;

    REQUIRE(ctx.evalStyleFn("fn_t", StyleParamKey::extrude, value) == true);
    REQUIRE(value.is<glm::vec2>() == true);
    StyleParam::Value e1(glm::vec2(NAN, NAN));
    REQUIRE(isnan(value.get<glm::vec2>()[0]) == true);

    REQUIRE(ctx.evalStyleFn("fn_f", StyleParamKey::extrude, value) == true);
    REQUIRE(value.is<glm::vec2>() == true);
    StyleParam::Value e2(glm::vec2(0.0f, 0.0f));
    REQUIRE(value == e2);

    REQUIRE(ctx.evalStyleFn("fn_a", StyleParamKey::extrude, value) == true);
    REQUIRE(value.is<glm::vec2>() == true);
    StyleParam::Value e3(glm::vec2(1.1f, 2.2f));
    REQUIRE(value == e3);

}

TEST_CASE( "Test evalFilter - Init filter function from yaml", "[Duktape][evalFilter]") {
    Scene scene;
    YAML::Node n0 = YAML::Load(R"(filter: function() { return feature.sort_key === 2; })");
    YAML::Node n1 = YAML::Load(R"(filter: function() { return feature.name === 'test'; })");

    Filter filter0 = SceneLoader::generateFilter(n0["filter"], scene);
    Filter filter1 = SceneLoader::generateFilter(n1["filter"], scene);

    REQUIRE(scene.functions().size() == 2);

    REQUIRE(filter0.type == FilterType::function);
    REQUIRE(filter1.type == FilterType::function);

    StyleContext ctx;
    ctx.initFunctions(scene);

    Feature feat1;
    feat1.props.add("sort_key", 2);
    feat1.props.add("name", "test");
    ctx.setFeature(feat1);

    // NB: feature parameter is ignored for Function evaluation
    REQUIRE(filter0.eval(feat1, ctx) == true);
    REQUIRE(filter1.eval(feat1, ctx) == true);

    // This is what happens in the above 'eval' internally
    REQUIRE(ctx.evalFilter(filter0.data.get<Filter::Function>().id) == true);
    REQUIRE(ctx.evalFilter(filter1.data.get<Filter::Function>().id) == true);

    // ... Also check that setFeature updates the ctx
    Feature feat2;
    feat2.props.add("name", "nope");
    ctx.setFeature(feat2);

    REQUIRE(filter0.eval(feat2, ctx) == false);
    REQUIRE(filter1.eval(feat2, ctx) == false);

    REQUIRE(ctx.evalFilter(filter0.data.get<Filter::Function>().id) == false);
    REQUIRE(ctx.evalFilter(filter1.data.get<Filter::Function>().id) == false);

}

TEST_CASE("Test evalStyle - Init StyleParam function from yaml", "[Duktape][evalStyle]") {
    Scene scene;
    YAML::Node n0 = YAML::Load(R"(
            draw:
                color: function() { return '#ffff00ff'; }
                width: function() { return 2; }
                cap: function() { return 'round'; }
            )");

    std::vector<StyleParam> styles;

    SceneLoader::parseStyleParams(n0["draw"], scene, "", styles);

    REQUIRE(scene.functions().size() == 3);

    // for (auto& str : scene.functions()) {
    //     logMsg("F: '%s'\n", str.c_str());
    // }

    StyleContext ctx;
    ctx.initFunctions(scene);

    for (auto& style : styles) {
        //logMsg("S: %d - '%s' %d\n", style.key, style.toString().c_str(), style.function);

        if (style.key == StyleParamKey::color) {
            StyleParam::Value value;
            REQUIRE(ctx.evalStyle(style.function, style.key, value) == true);
            REQUIRE(value.is<uint32_t>() == true);
            REQUIRE(value.get<uint32_t>() == 0xffff00ff);

        } else if (style.key == StyleParamKey::width) {
            StyleParam::Value value;
            REQUIRE(ctx.evalStyle(style.function, style.key, value) == true);
            REQUIRE(value.is<StyleParam::Width>() == true);
            REQUIRE(value.get<StyleParam::Width>().value == 2);

        } else if (style.key == StyleParamKey::cap) {
            StyleParam::Value value;
            REQUIRE(ctx.evalStyle(style.function, style.key, value) == true);
            REQUIRE(value.is<uint32_t>() == true);
            REQUIRE(static_cast<CapTypes>(value.get<uint32_t>()) == CapTypes::round);
        } else {
            REQUIRE(true == false);
        }
    }
}
