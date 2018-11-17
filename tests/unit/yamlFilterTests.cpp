#include "catch.hpp"

#include "data/tileData.h"
#include "mockPlatform.h"
#include "scene/filters.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "scene/styleContext.h"

#include "yaml-cpp/yaml.h"

#include <iostream>
#include <vector>

using namespace Tangram;
using YAML::Node;

using Context = StyleContext;

Context ctx;

Feature civic, bmw1, bike;

Filter load(const std::string& filterYaml) {
    MockPlatform platform;
    Scene scene(platform);
    YAML::Node node = YAML::Load(filterYaml);
    auto filter = SceneLoader::generateFilter(scene, node["filter"]);
    ctx.initFunctions(scene);
    return filter;
}

void init() {

    civic.props.clear();
    civic.props.set("name", "civic");
    civic.props.set("brand", "honda");
    civic.props.set("wheel",  4);
    civic.props.set("drive", "fwd");
    civic.props.set("type", "car");

    bmw1.props.clear();
    bmw1.props.set("name", "bmw320i");
    bmw1.props.set("brand", "bmw");
    bmw1.props.set("check", "false");
    bmw1.props.set("series", "3");
    bmw1.props.set("wheel", 4);
    bmw1.props.set("drive", "all");
    bmw1.props.set("type", "car");
    bmw1.props.set("serial", 4398046511104); // 2^42

    bike.props.clear();
    bike.props.set("name", "cb1100");
    bike.props.set("brand", "honda");
    bike.props.set("wheel", 2);
    bike.props.set("type", "bike");
    bike.props.set("series", "CB");
    bike.props.set("check", "available");
    bike.props.set("serial", 4398046511105); // 2^42 + 1

    ctx.setKeyword("$geometry", Value(1));
    ctx.setKeyword("$zoom", Value("false"));
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
    Filter filter = load("filter: {not : { any: [{name : civic}, {name : bmw320i}]}}");

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

//10. basic predicate with context
TEST_CASE( "yaml-filter-tests: context filter", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {$geometry : 1}");

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
    Filter filter = load("filter: {$geometry : 1}");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: boolean false filter as existence check for keyword", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: {$zoom : false}");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: predicate with large integers", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: { serial : [4398046511104] }");

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

TEST_CASE("Filters specified as a javascript function evaluate correctly", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: 'function() { return false; }'");

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));
}

TEST_CASE("Sequences in filters implicitly create an 'any' filter", "[filters][core][yaml]") {
    init();
    Filter filter = load("filter: [ { brand: 'bmw' }, { type: 'car' } ]");

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));
}
