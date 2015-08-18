#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <vector>

#include "data/filters.h"
#include "yaml-cpp/yaml.h"
#include "scene/sceneLoader.h"

using namespace Tangram;
using YAML::Node;

SceneLoader sceneLoader;
Context ctx;

Feature civic, bmw1, bike;

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

    ctx["$vroom"] = Value(1);
    ctx["$zooooom"] = Value("false");
}


//1. basic predicate
TEST_CASE( "yaml-filter-tests: basic predicate test", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: { series: 3}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//2. predicate with valueList
TEST_CASE( "yaml-filter-tests: predicate with valueList", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: { name : [civic, bmw320i] }");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//3. range min
TEST_CASE( "yaml-filter-tests: range min", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: {wheel : {min : 3}}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//4. range max
TEST_CASE( "yaml-filter-tests: range max", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: {wheel : {max : 2}}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//5. range min max
TEST_CASE( "yaml-filter-tests: range min max", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: {wheel : {min : 2, max : 5}}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

//6. any
TEST_CASE( "yaml-filter-tests: any", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: {any : [{name : civic}, {name : bmw320i}]}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//7. all
TEST_CASE( "yaml-filter-tests: all", "[filters][core][yaml]") {
    init();
    //YAML::Node node = YAML::Load("filter: {any : [{name : civic}, {name : bmw320i}]}");
    YAML::Node node = YAML::Load("filter: {all : [ {name : civic}, {brand : honda}, {wheel: 4} ] }");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

//8. none
TEST_CASE( "yaml-filter-tests: none", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: {none : [{name : civic}, {name : bmw320i}]}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

//9. not
TEST_CASE( "yaml-filter-tests: not", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: {not : {name : civic}}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

//10. basic predicate with context
TEST_CASE( "yaml-filter-tests: context filter", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: {$vroom : 1}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: bogus filter", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: {max: bogus}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: boolean true filter as existence check", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: { drive : true }");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(!filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: boolean false filter as existence check", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: { drive : false}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(!filter.eval(civic, ctx));
    REQUIRE(!filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: boolean true filter as existence check for keyword", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: {$vroom : true}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

TEST_CASE( "yaml-filter-tests: boolean false filter as existence check for keyword", "[filters][core][yaml]") {
    init();
    YAML::Node node = YAML::Load("filter: {$foo : false}");
    Filter filter = sceneLoader.generateFilter(node["filter"]);

    REQUIRE(filter.eval(civic, ctx));
    REQUIRE(filter.eval(bmw1, ctx));
    REQUIRE(filter.eval(bike, ctx));

}

