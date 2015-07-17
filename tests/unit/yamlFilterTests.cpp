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

    civic.props.stringProps.clear();
    civic.props.numericProps.clear();
    civic.props.stringProps["name"] = "civic";
    civic.props.stringProps["brand"] = "honda";
    civic.props.numericProps["wheel"] = 4;
    civic.props.stringProps["drive"] = "fwd";
    civic.props.stringProps["type"] = "car";

    bmw1.props.stringProps.clear();
    bmw1.props.numericProps.clear();
    bmw1.props.stringProps["name"] = "bmw320i";
    bmw1.props.stringProps["brand"] = "bmw";
    bmw1.props.stringProps["check"] = "false";
    bmw1.props.stringProps["series"] = "3";
    bmw1.props.numericProps["wheel"] = 4;
    bmw1.props.stringProps["drive"] = "all";
    bmw1.props.stringProps["type"] = "car";

    bike.props.stringProps.clear();
    bike.props.numericProps.clear();
    bike.props.stringProps["name"] = "cb1100";
    bike.props.stringProps["brand"] = "honda";
    bike.props.numericProps["wheel"] = 2;
    bike.props.stringProps["type"] = "bike";
    bike.props.stringProps["series"] = "CB";
    bike.props.stringProps["check"] = "available";

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

