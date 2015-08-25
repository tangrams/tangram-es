#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "filterContext.h"
#include "data/filters.h"
#include "yaml-cpp/yaml.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"

using namespace Tangram;

SceneLoader sceneLoader;


TEST_CASE( "", "[Duktape][init]") {
    {
        FilterContext();
    }
}

TEST_CASE( "Test evalFilterFn with feature", "[Duktape][evalFilterFn]") {
    {
        Feature feature;
        feature.props.add("a", "A");
        feature.props.add("b", "B");
        feature.props.add("n", 42);

        FilterContext ctx;
        ctx.setFeature(feature);

        ctx.addFilterFn("fn_a", R"(function() { return feature.a == 'A' })");
        REQUIRE(ctx.evalFilterFn("fn_a") == true);

        ctx.addFilterFn("fn_b", R"(function() { return feature.b == 'B' })");
        REQUIRE(ctx.evalFilterFn("fn_b") == true);

        ctx.addFilterFn("fn_n", R"(function() { return feature.n == 42 })");
        REQUIRE(ctx.evalFilterFn("fn_n") == true);

        ctx.addFilterFn("fn_n2", R"(function() { return feature.n == 43 })");
        REQUIRE(ctx.evalFilterFn("fn_n2") == false);

        // OK?
        ctx.addFilterFn("fn_n3", R"(function() { return feature.n == '42' })");
        REQUIRE(ctx.evalFilterFn("fn_n3") == true);
    }
}

TEST_CASE( "Test evalFilterFn with feature and globals", "[Duktape][evalFilterFn]") {
    {
        Feature feature;
        feature.props.add("scalerank", 2);

        FilterContext ctx;
        ctx.setFeature(feature);
        ctx.setGlobal("$zoom", 5);

        ctx.addFilterFn("fn", R"(function() { return (feature.scalerank * .5) <= ($zoom - 4); })");
        REQUIRE(ctx.evalFilterFn("fn") == true);

        ctx.setGlobal("$zoom", 4);
        REQUIRE(ctx.evalFilterFn("fn") == false);
    }
}

TEST_CASE( "Test evalFilterFn with different features", "[Duktape][evalFilterFn]") {
    {
        FilterContext ctx;

        ctx.addFilterFn("fn", R"(function() { return feature.scalerank == 2; })");

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
}
TEST_CASE( "Test numeric global", "[Duktape][setGlobal]") {
    {
        FilterContext ctx;
        ctx.setGlobal("$zoom", 10);
        ctx.addFilterFn("fn", R"(function() { return $zoom == 10 })");

        REQUIRE(ctx.evalFilterFn("fn") == true);

        ctx.setGlobal("$zoom", 0);

        REQUIRE(ctx.evalFilterFn("fn") == false);
    }
}

TEST_CASE( "Test string global", "[Duktape][setGlobal]") {
    {
        FilterContext ctx;
        ctx.setGlobal("$layer", "test");
        ctx.addFilterFn("fn", R"(function() { return $layer == 'test' })");

        REQUIRE(ctx.evalFilterFn("fn") == true);

        ctx.setGlobal("$layer", "none");

        REQUIRE(ctx.evalFilterFn("fn") == false);
    }
}

TEST_CASE( "Test evalStyleFn - StyleParamKey::order", "[Duktape][evalStyleFn]") {
    {
        Feature feat;
        feat.props.add("sort_key", 2);

        FilterContext ctx;
        ctx.setFeature(feat);
        ctx.addFilterFn("fn", R"(function () { return feature.sort_key + 5 })");

        StyleParam::Value value;

        REQUIRE(ctx.evalStyleFn("fn", StyleParamKey::order, value) == true);
        REQUIRE(value.is<int32_t>() == true);
        REQUIRE(value.get<int32_t>() == 7);
    }
}

TEST_CASE( "Test evalStyleFn - StyleParamKey::color", "[Duktape][evalStyleFn]") {
    {
        Feature feat;
        feat.props.add("sort_key", 2);

        FilterContext ctx;
        ctx.setFeature(feat);
        ctx.addFilterFn("fn", R"(function () { return '#f0f' })");

        StyleParam::Value value;

        REQUIRE(ctx.evalStyleFn("fn", StyleParamKey::color, value) == true);
        REQUIRE(value.is<Color>() == true);
        REQUIRE(value.get<Color>().getInt() == 0xffff00ff);

        // ctx.addFilterFn("fn2", R"(function () { return [1., 1., 0.] })");
        // REQUIRE(ctx.evalStyleFn("fn2", StyleParamKey::color, value) == true);
        // REQUIRE(value.is<Color>() == true);
        // REQUIRE(value.get<Color>().getInt() == 0xffffff00);
    }
}

TEST_CASE( "Test evalFilterFn - parse filter", "[Duktape][evalFilterFn]") {
    {
        Scene scene;
        YAML::Node node = YAML::Load("filter: function() { return true }");
        Filter filter = sceneLoader.generateFilter(node["filter"], scene);

        REQUIRE(filter.type == FilterType::function);

        Feature feat;
        feat.props.add("sort_key", 2);

        FilterContext ctx;
        ctx.setFeature(feat);
        ctx.addFilterFn("fn", R"(function () { return '#f0f' })");

        StyleParam::Value value;

        REQUIRE(ctx.evalStyleFn("fn", StyleParamKey::color, value) == true);
        REQUIRE(value.is<Color>() == true);
        REQUIRE(value.get<Color>().getInt() == 0xffff00ff);
    }
}

TEST_CASE( "Test evalFilterFn - init function from scene", "[Duktape][evalFilterFn]") {
    {
        Scene scene;
        YAML::Node node = YAML::Load(R"(filter: function() { return feature.sort_key == 2; })");
        Filter filter = sceneLoader.generateFilter(node["filter"], scene);
        REQUIRE(filter.type == FilterType::function);


        FilterContext ctx;
        ctx.initFunctions(scene);

        Feature feat1;
        feat1.props.add("sort_key", 2);
        ctx.setFeature(feat1);

        REQUIRE(ctx.evalFilter(0) == true);

        Feature feat2;
        ctx.setFeature(feat2);

        REQUIRE(ctx.evalFilter(0) == false);
    }
}
