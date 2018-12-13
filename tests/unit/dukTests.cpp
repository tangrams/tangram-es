#include "catch.hpp"

#include "scene/filters.h"
#include "scene/sceneLoader.h"
#include "scene/styleContext.h"
#include "util/builders.h"

#include "yaml-cpp/yaml.h"

using namespace Tangram;

TEST_CASE( "", "[Duktape][init]") {
    StyleContext();
}

TEST_CASE( "Test evalFilterFn with feature", "[Duktape][evalFilterFn]") {
    Feature feature;
    feature.props.set("a", "A");
    feature.props.set("b", "B");
    feature.props.set("n", 42);

    StyleContext ctx;
    ctx.setFeature(feature);

    REQUIRE(ctx.setFunctions({R"(function() { return feature.a === 'A' })"}));
    REQUIRE(ctx.evalFilter(0) == true);

    REQUIRE(ctx.setFunctions({ R"(function() { return feature.b === 'B' })"}));
    REQUIRE(ctx.evalFilter(0) == true);

    REQUIRE(ctx.setFunctions({ R"(function() { return feature.n === 42 })"}));
    REQUIRE(ctx.evalFilter(0) == true);

    REQUIRE(ctx.setFunctions({ R"(function() { return feature.n === 43 })"}));
    REQUIRE(ctx.evalFilter(0) == false);

    REQUIRE(ctx.setFunctions({ R"(function() { return feature.n === '42' })"}));
    REQUIRE(ctx.evalFilter(0) == false);
}

TEST_CASE( "Test evalFilterFn with feature and keywords", "[Duktape][evalFilterFn]") {
    Feature feature;
    feature.props.set("scalerank", 2);

    StyleContext ctx;
    ctx.setFeature(feature);
    ctx.setKeyword("$zoom", 5);

    REQUIRE(ctx.setFunctions({ R"(function() { return (feature.scalerank * .5) <= ($zoom - 4); })"}));
    REQUIRE(ctx.evalFilter(0) == true);

    ctx.setKeyword("$zoom", 4);
    REQUIRE(ctx.evalFilter(0) == false);

}

TEST_CASE( "Test evalFilterFn with feature and keyword geometry", "[Duktape][evalFilterFn]") {
    Feature points;
    points.geometryType = GeometryType::points;

    Feature lines;
    lines.geometryType = GeometryType::lines;

    Feature polygons;
    polygons.geometryType = GeometryType::polygons;

    StyleContext ctx;

    // Test $geometry keyword
    REQUIRE(ctx.setFunctions({
                R"(function() { return $geometry === 'point'; })",
                R"(function() { return $geometry === 'line'; })",
                R"(function() { return $geometry === 'polygon'; })"}));

    ctx.setFeature(points);
    REQUIRE(ctx.evalFilter(0) == true);
    REQUIRE(ctx.evalFilter(1) == false);
    REQUIRE(ctx.evalFilter(2) == false);

    ctx.setFeature(lines);
    REQUIRE(ctx.evalFilter(0) == false);
    REQUIRE(ctx.evalFilter(1) == true);
    REQUIRE(ctx.evalFilter(2) == false);

    ctx.setFeature(polygons);
    REQUIRE(ctx.evalFilter(0) == false);
    REQUIRE(ctx.evalFilter(1) == false);
    REQUIRE(ctx.evalFilter(2) == true);

}

TEST_CASE( "Test evalFilterFn with different features", "[Duktape][evalFilterFn]") {
    StyleContext ctx;

    REQUIRE(ctx.setFunctions({ R"(function() { return feature.scalerank === 2; })"}));

    Feature feat1;
    feat1.props.set("scalerank", 2);

    ctx.setFeature(feat1);
    REQUIRE(ctx.evalFilter(0) == true);

    Feature feat2;
    ctx.setFeature(feat2);
    REQUIRE(ctx.evalFilter(0) == false);

    ctx.setFeature(feat1);
    REQUIRE(ctx.evalFilter(0) == true);
}

TEST_CASE( "Test numeric keyword", "[Duktape][setKeyword]") {
    StyleContext ctx;
    ctx.setKeyword("$zoom", 10);
    REQUIRE(ctx.setFunctions({ R"(function() { return $zoom === 10 })"}));
    REQUIRE(ctx.evalFilter(0) == true);

    ctx.setKeyword("$zoom", 0);
    REQUIRE(ctx.evalFilter(0) == false);
}

TEST_CASE( "Test string keyword", "[Duktape][setKeyword]") {
    StyleContext ctx;
    ctx.setKeyword("$geometry", GeometryType::points);
    REQUIRE(ctx.setFunctions({ R"(function() { return $geometry === point })"}));
    REQUIRE(ctx.evalFilter(0) == true);

    ctx.setKeyword("$geometry", "none");
    REQUIRE(ctx.evalFilter(0) == false);

}

TEST_CASE( "Test evalStyleFn - StyleParamKey::order", "[Duktape][evalStyleFn]") {
    Feature feat;
    feat.props.set("sort_key", 2);

    StyleContext ctx;
    ctx.setFeature(feat);
    REQUIRE(ctx.setFunctions({ R"(function () { return feature.sort_key + 5 })"}));

    StyleParam::Value value;

    REQUIRE(ctx.evalStyle(0, StyleParamKey::order, value) == true);
    REQUIRE(value.is<uint32_t>() == true);
    REQUIRE(value.get<uint32_t>() == 7);
}

TEST_CASE( "Test evalStyleFn - StyleParamKey::color", "[Duktape][evalStyleFn]") {

    StyleContext ctx;
    StyleParam::Value value;

    REQUIRE(ctx.setFunctions({ R"(function () { return '#f0f'; })"}));
    REQUIRE(ctx.evalStyle(0, StyleParamKey::color, value) == true);
    REQUIRE(value.is<uint32_t>() == true);
    REQUIRE(value.get<uint32_t>() == 0xffff00ff);

    REQUIRE(ctx.setFunctions({ R"(function () { return 0xff00ffff; })"}));
    REQUIRE(ctx.evalStyle(0, StyleParamKey::color, value) == true);
    REQUIRE(value.is<uint32_t>() == true);
    REQUIRE(value.get<uint32_t>() == 0xff00ffff);

    REQUIRE(ctx.setFunctions({ R"(function () { return [1.0, 1.0, 0.0, 1.0] })"}));
    REQUIRE(ctx.evalStyle(0, StyleParamKey::color, value) == true);
    REQUIRE(value.is<uint32_t>() == true);
    REQUIRE(value.get<uint32_t>() == 0xff00ffff);

    REQUIRE(ctx.setFunctions({ R"(function () { return [0.0, 1.0, 0.0] })"}));
    REQUIRE(ctx.evalStyle(0, StyleParamKey::color, value) == true);
    REQUIRE(value.is<uint32_t>() == true);
    REQUIRE(value.get<uint32_t>() == 0xff00ff00);

}

TEST_CASE( "Test evalStyleFn - StyleParamKey::width", "[Duktape][evalStyleFn]") {
    Feature feat;
    feat.props.set("width", 2.0);

    StyleContext ctx;
    ctx.setFeature(feat);
    REQUIRE(ctx.setFunctions({ R"(function () { return feature.width * 2.3; })"}));

    StyleParam::Value value;

    REQUIRE(ctx.evalStyle(0, StyleParamKey::width, value) == true);
    REQUIRE(value.is<StyleParam::Width>() == true);
    REQUIRE(value.get<StyleParam::Width>().value == 4.6f);
}

TEST_CASE( "Test evalStyleFn - StyleParamKey::extrude", "[Duktape][evalStyleFn]") {
    Feature feat;
    feat.props.set("width", 2.0);

    StyleContext ctx;
    ctx.setFeature(feat);
    REQUIRE(ctx.setFunctions({
                R"(function () { return true; })",
                R"(function () { return false; })",
                R"(function () { return [1.1, 2.2]; })"}));

    StyleParam::Value value;

    REQUIRE(ctx.evalStyle(0, StyleParamKey::extrude, value) == true);
    REQUIRE(value.is<glm::vec2>() == true);
    StyleParam::Value e1(glm::vec2(NAN, NAN));
    REQUIRE(std::isnan(value.get<glm::vec2>()[0]) == true);

    REQUIRE(ctx.evalStyle(1, StyleParamKey::extrude, value) == true);
    REQUIRE(value.is<glm::vec2>() == true);
    StyleParam::Value e2(glm::vec2(0.0f, 0.0f));
    REQUIRE(value == e2);

    REQUIRE(ctx.evalStyle(2, StyleParamKey::extrude, value) == true);
    REQUIRE(value.is<glm::vec2>() == true);
    StyleParam::Value e3(glm::vec2(1.1f, 2.2f));
    REQUIRE(value == e3);

}

TEST_CASE( "Test evalStyleFn - StyleParamKey::text_source", "[Duktape][evalStyleFn]") {
    Feature feat;
    feat.props.set("name", "my name is my name");

    StyleContext ctx;
    ctx.setFeature(feat);
    REQUIRE(ctx.setFunctions({
                R"(function () { return 'hello!'; })",
                R"(function () { return feature.name; })"}));

    StyleParam::Value value;

    REQUIRE(ctx.evalStyle(0, StyleParamKey::text_source, value) == true);
    REQUIRE(value.is<std::string>());
    REQUIRE(value.get<std::string>() == "hello!");

    REQUIRE(ctx.evalStyle(1, StyleParamKey::text_source, value) == true);
    REQUIRE(value.is<std::string>());
    REQUIRE(value.get<std::string>() == "my name is my name");
}

TEST_CASE( "Test evalFilter - Init filter function from yaml", "[Duktape][evalFilter]") {
    SceneFunctions fns;
    YAML::Node n0 = YAML::Load(R"(filter: function() { return feature.sort_key === 2; })");
    YAML::Node n1 = YAML::Load(R"(filter: function() { return feature.name === 'test'; })");

    Filter filter0 = SceneLoader::generateFilter(fns, n0["filter"]);
    Filter filter1 = SceneLoader::generateFilter(fns, n1["filter"]);

    REQUIRE(fns.size() == 2);

    REQUIRE(filter0.data.is<Filter::Function>());
    REQUIRE(filter1.data.is<Filter::Function>());

    StyleContext ctx;
    ctx.setFunctions(fns);

    Feature feat1;
    feat1.props.set("sort_key", 2);
    feat1.props.set("name", "test");
    ctx.setFeature(feat1);

    // NB: feature parameter is ignored for Function evaluation
    REQUIRE(filter0.eval(feat1, ctx) == true);
    REQUIRE(filter1.eval(feat1, ctx) == true);

    // This is what happens in the above 'eval' internally
    REQUIRE(ctx.evalFilter(filter0.data.get<Filter::Function>().id) == true);
    REQUIRE(ctx.evalFilter(filter1.data.get<Filter::Function>().id) == true);

    // ... Also check that setFeature updates the ctx
    Feature feat2;
    feat2.props.set("name", "nope");
    ctx.setFeature(feat2);

    REQUIRE(filter0.eval(feat2, ctx) == false);
    REQUIRE(filter1.eval(feat2, ctx) == false);

    REQUIRE(ctx.evalFilter(filter0.data.get<Filter::Function>().id) == false);
    REQUIRE(ctx.evalFilter(filter1.data.get<Filter::Function>().id) == false);

}

TEST_CASE("Test evalStyle - Init StyleParam function from yaml", "[Duktape][evalStyle]") {
    SceneStops stops;
    SceneFunctions fns;

    YAML::Node n0 = YAML::Load(R"(
            draw:
                color: function() { return '#ffff00ff'; }
                width: function() { return 2; }
                cap: function() { return 'round'; }
            )");

    std::vector<StyleParam> styles;

    SceneLoader::parseStyleParams(stops, fns, n0["draw"], "", styles);

    REQUIRE(fns.size() == 3);

    // for (auto& str : scene.functions()) {
    //     logMsg("F: '%s'\n", str.c_str());
    // }

    StyleContext ctx;
    ctx.setFunctions(fns);

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

TEST_CASE( "Test evalFunction explicit", "[Duktape][evalFunction]") {
    YAML::Node n0 = YAML::Load(R"(
            global:
                width: 2
                mapNode:
                    color: function(c) { return c; }
                    caps:
                        cap: round
                    test: function
            draw:
                color: function() { return global.mapNode.color("blue"); }
                width: function() { return global.width; }
                cap: function() { return global.mapNode.caps.cap; }
                text_source: function() { return global.mapNode.test; }
            )");

    std::vector<StyleParam> styles;

    SceneStops stops;
    SceneFunctions fns;
    SceneLoader::parseStyleParams(stops, fns, n0["draw"], "", styles);

    REQUIRE(fns.size() == 4);

    StyleContext ctx;
    ctx.setFunctions(fns);

    for (auto& style : styles) {
        if (style.key == StyleParamKey::color) {
            StyleParam::Value value;
            REQUIRE(ctx.evalStyle(style.function, style.key, value) == true);
            REQUIRE(value.is<uint32_t>() == true);
            REQUIRE(value.get<uint32_t>() == 0xffff0000);

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

        } else if(style.key == StyleParamKey::text_source) {
            StyleParam::Value value;
            REQUIRE(ctx.evalStyle(style.function, style.key, value) == true);
            REQUIRE(value.is<std::string>() == true);
            REQUIRE(value.get<std::string>() == "function");

        } else {
            REQUIRE(true == false);
        }
    }

}
