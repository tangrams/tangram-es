#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <vector>

#include "yaml-cpp/yaml.h"
#include "scene/filters.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"

using namespace Tangram;
using YAML::Node;

using Context = StyleContext;

Context ctx;

Feature civic, bmw1, bike;

Filter load(const std::string& filterYaml) {
    Scene scene;
    YAML::Node node = YAML::Load(filterYaml);
    return SceneLoader::generateFilter(node["filter"], scene);
}

void init() {

    civic.props.clear();
    civic.props.add("name", "civic");
    civic.props.add("brand", "honda");
    civic.props.add("wheel",  4);
    civic.props.add("drive", "fwd");
    civic.props.add("type", "car");

    bmw1.props.clear();
    bmw1.props.add("name", "bmw320i");
    bmw1.props.add("brand", "bmw");
    bmw1.props.add("check", "false");
    bmw1.props.add("series", "3");
    bmw1.props.add("wheel", 4);
    bmw1.props.add("drive", "all");
    bmw1.props.add("type", "car");

    bike.props.clear();
    bike.props.add("name", "cb1100");
    bike.props.add("brand", "honda");
    bike.props.add("wheel", 2);
    bike.props.add("type", "bike");
    bike.props.add("series", "CB");
    bike.props.add("check", "available");

    ctx.setGlobal("$vroom", Value(1));
    ctx.setGlobal("$zooooom", Value("false"));
}


//1. basic predicate
TEST_CASE( "yaml-filter-tests: basic predicate test", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: { series: !!str 3}");

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//2. predicate with valueList
TEST_CASE( "yaml-filter-tests: predicate with valueList", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: { name : [civic, bmw320i] }");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//3. range min
TEST_CASE( "yaml-filter-tests: range min", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {wheel : {min : 3}}");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//4. range max
TEST_CASE( "yaml-filter-tests: range max", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {wheel : {max : 2}}");

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//5. range min max
TEST_CASE( "yaml-filter-tests: range min max", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {wheel : {min : 2, max : 5}}");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

//6. any
TEST_CASE( "yaml-filter-tests: any", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {any : [{name : civic}, {name : bmw320i}]}");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//7. all
TEST_CASE( "yaml-filter-tests: all", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {all : [ {name : civic}, {brand : honda}, {wheel: 4} ] }");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//8. none
TEST_CASE( "yaml-filter-tests: none", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {none : [{name : civic}, {name : bmw320i}]}");

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

//9. not
TEST_CASE( "yaml-filter-tests: not", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {not : {name : civic}}");

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

//10. basic predicate with context
TEST_CASE( "yaml-filter-tests: context filter", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {$vroom : 1}");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: bogus filter", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {max: bogus}");

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: boolean true filter as existence check", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: { drive : true }");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: boolean false filter as existence check", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: { drive : false}");

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: boolean true filter as existence check for keyword", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {$vroom : true}");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: boolean false filter as existence check for keyword", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {$foo : false}");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

