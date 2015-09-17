#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "scene/stops.h"

using namespace Tangram;

Stops instance_float() {
    return Stops({
        Frame(0, 0.f),
        Frame(1, 10.f),
        Frame(5, 50.f),
        Frame(7, 0.f)
    });
}

Stops instance_color() {
    return Stops({
        Frame(0, Color(0xff, 0xff, 0xff, 1.)),
        Frame(1, Color(0xee, 0xee, 0xee, 1.)),
        Frame(5, Color(0xaa, 0xaa, 0xaa, 1.))
    });
}

TEST_CASE( "Stops evaluate float values correctly at and between key frames", "[Stops]" ) {

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

TEST_CASE( "Stops evaluate color values correctly at and between key frames", "[Stops]" ) {

    auto stops = instance_color();

    REQUIRE(stops.evalColor(-1) == 0xffffffff);
    REQUIRE(stops.evalColor(1) == 0xffeeeeee);
    REQUIRE(stops.evalColor(2) == 0xffdddddd);
    REQUIRE(stops.evalColor(7) == 0xffaaaaaa);

}
