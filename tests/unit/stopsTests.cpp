#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "scene/stops.h"
#include "yaml-cpp/yaml.h"

using namespace Tangram;

Stops instance_float() {
    return Stops({
        Stops::Frame(0, 0.f),
        Stops::Frame(1, 10.f),
        Stops::Frame(5, 50.f),
        Stops::Frame(7, 0.f)
    });
}

Stops instance_color() {
    return Stops({
        Stops::Frame(0, 0xffffffff),
        Stops::Frame(1, 0xffeeeeee),
        Stops::Frame(5, 0xffaaaaaa)
    });
}

TEST_CASE("Stops evaluate float values correctly at and between key frames", "[Stops]") {

    auto stops = instance_float();

    REQUIRE(stops.evalFloat(-3) == 0.f);
    REQUIRE(stops.evalFloat(0) == 0.f);
    REQUIRE(stops.evalFloat(0.3) == 3.f);
    REQUIRE(stops.evalFloat(1) == 10.f);
    REQUIRE(stops.evalFloat(3) == 30.f);
    REQUIRE(stops.evalFloat(5) == 50.f);
    REQUIRE(stops.evalFloat(6) == 25.f);
    REQUIRE(stops.evalFloat(7) == 0.f);
    REQUIRE(stops.evalFloat(8) == 0.f);

}

TEST_CASE("Stops evaluate color values correctly at and between key frames", "[Stops]") {

    auto stops = instance_color();

    REQUIRE(stops.evalColor(-1) == 0xffffffff);
    REQUIRE(stops.evalColor(1) == 0xffeeeeee);
    REQUIRE(stops.evalColor(2) == 0xffdddddd);
    REQUIRE(stops.evalColor(7) == 0xffaaaaaa);

}

TEST_CASE("Stops parses correctly from YAML distance values", "[Stops][YAML]") {

    YAML::Node node = YAML::Load("[ [10, 0], [16, .04], [18, .2] ]");

    Stops stops(Stops::Width(node));

    REQUIRE(stops.frames.size() == 3);
    REQUIRE(stops.frames[0].key == 10.f);
    REQUIRE(stops.frames[1].key == 16.f);
    REQUIRE(stops.frames[2].key == 18.f);
    REQUIRE(stops.frames[0].width.value == 0.f);
    REQUIRE(stops.frames[1].width.value == .04f);
    REQUIRE(stops.frames[2].width.value == .2f);

}

TEST_CASE("Stops parses correctly from YAML color values", "[Stops][YAML]") {

    YAML::Node node = YAML::Load("[ [10, '#aaa'], [16, [0, .5, 1] ], [18, [0, .25, 1, .5] ] ]");

    Stops stops(Stops::Color(node));

    REQUIRE(stops.frames.size() == 3);
    REQUIRE(stops.frames[0].key == 10.f);
    REQUIRE(stops.frames[1].key == 16.f);
    REQUIRE(stops.frames[2].key == 18.f);
    REQUIRE(stops.frames[0].color == 0xffaaaaaa);
    REQUIRE(stops.frames[1].color == 0xffff7f00);
    REQUIRE(stops.frames[2].color == 0x7fff3f00);

}
