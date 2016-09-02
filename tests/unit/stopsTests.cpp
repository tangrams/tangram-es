#include "catch.hpp"

#include "scene/stops.h"
#include "scene/styleParam.h"
#include "yaml-cpp/yaml.h"
#include "util/mapProjection.h"

using namespace Tangram;

Stops instance_color() {
    return Stops({
        Stops::Frame(0, Color(0xffffffff)),
        Stops::Frame(1, Color(0xffeeeeee)),
        Stops::Frame(5, Color(0xffaaaaaa))
    });
}

TEST_CASE("Stops evaluate float values correctly at and between key frames", "[Stops]") {

    Stops stops({
            Stops::Frame(0, 0.f),
            Stops::Frame(1, 10.f),
            Stops::Frame(5, 50.f),
            Stops::Frame(7, 0.f)
    });

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

TEST_CASE("Stops evaluate vec2 values correctly at and between key frames", "[Stops]") {

    Stops stops({
            Stops::Frame(0, glm::vec2(0.0)),
            Stops::Frame(1, glm::vec2(1.0)),
            Stops::Frame(2, glm::vec2(1.0, 0.0)),
            Stops::Frame(4, glm::vec2(0.0, 1.0))
    });

    REQUIRE(stops.evalVec2(-3) == glm::vec2(0.0));
    REQUIRE(stops.evalVec2(0) == glm::vec2(0.0));
    REQUIRE(stops.evalVec2(0.3) == glm::vec2(0.3));
    REQUIRE(stops.evalVec2(1) == glm::vec2(1.0));
    REQUIRE(stops.evalVec2(1.5) == glm::vec2(1.0, 0.5));
    REQUIRE(stops.evalVec2(4) == glm::vec2(0.0, 1.0));
    REQUIRE(stops.evalVec2(3.0) == glm::vec2(0.5));;

}



TEST_CASE("Stops evaluate width values correctly at and between key frames", "[Stops]") {

    Stops stops({
            Stops::Frame(0, 0.f),
            Stops::Frame(1, 10.f),
            Stops::Frame(5, 50.f),
            Stops::Frame(7, 0.f),
            Stops::Frame(8, 3.f),
            Stops::Frame(10, 3.f),

    });

    REQUIRE(stops.evalWidth(-3) == 0.f);
    REQUIRE(stops.evalWidth(0) == 0.f);
    REQUIRE(std::abs(stops.evalWidth(0.3) - 2.31144f) < 0.00001);
    REQUIRE(stops.evalWidth(1) == 10.f);
    REQUIRE(stops.evalWidth(3) == 18.f);
    REQUIRE(stops.evalWidth(5) == 50.f);
    REQUIRE(std::abs(stops.evalWidth(6) - 33.33333f) < 0.00001);
    REQUIRE(stops.evalWidth(7) == 0.f);
    REQUIRE(stops.evalWidth(8) == 3.f);
    REQUIRE(stops.evalWidth(8.4) == 3.f); // flat interpolation
    REQUIRE(stops.evalWidth(9.3) == 3.f); // flat interpolation
    REQUIRE(stops.evalWidth(10) == 3.f);

}

TEST_CASE("Stops evaluate color values correctly at and between key frames", "[Stops]") {

    auto stops = instance_color();

    REQUIRE(stops.evalColor(-1) == 0xffffffff);
    REQUIRE(stops.evalColor(1) == 0xffeeeeee);
    REQUIRE(stops.evalColor(2) == 0xffdddddd);
    REQUIRE(stops.evalColor(7) == 0xffaaaaaa);

}

TEST_CASE("Stops parses correctly from YAML distance values", "[Stops][YAML]") {

    YAML::Node node = YAML::Load("[ [10, 0], [16, .04], [18, .2], [19, .2] ]");

    MercatorProjection proj;

    Stops stops(Stops::Widths(node, proj.TileSize(), {}));

    // +1 added for meter end stop
    REQUIRE(stops.frames.size() == 5);
    REQUIRE(stops.frames[0].key == 10.f);
    REQUIRE(stops.frames[1].key == 16.f);
    REQUIRE(stops.frames[2].key == 18.f);
    REQUIRE(stops.frames[3].key == 19.f);
    REQUIRE(stops.frames[0].value.get<float>() == 0.f);

    // check if same meters have twice the width in pixel one zoom-level above
    REQUIRE(std::abs(stops.frames[2].value.get<float>() * 2.0 - stops.frames[3].value.get<float>()) < 0.00001);
}

TEST_CASE("Stops parses correctly from YAML color values", "[Stops][YAML]") {

    YAML::Node node = YAML::Load("[ [10, '#aaa'], [16, [0, .5, 1] ], [18, [0, .25, 1, .5] ] ]");

    Stops stops(Stops::Colors(node));

    REQUIRE(stops.frames.size() == 3);
    REQUIRE(stops.frames[0].key == 10.f);
    REQUIRE(stops.frames[1].key == 16.f);
    REQUIRE(stops.frames[2].key == 18.f);
    REQUIRE(stops.frames[0].value.get<Color>().abgr == 0xffaaaaaa);
    REQUIRE(stops.frames[1].value.get<Color>().abgr == 0xffff7f00);
    REQUIRE(stops.frames[2].value.get<Color>().abgr == 0x7fff3f00);

}

TEST_CASE("Regression test - Dont crash on evaluating empty stops", "[Stops][YAML]") {

    YAML::Node node = YAML::Load("[]");

    {
        MercatorProjection proj{};
        std::vector<Unit> units = { Unit::meter };
        Stops stops(Stops::Widths(node, proj.TileSize(), units));
        REQUIRE(stops.frames.size() == 0);
        stops.evalVec2(1);
    }
    {
        Stops stops(Stops::Colors(node));
        REQUIRE(stops.frames.size() == 0);
        stops.evalVec2(1);
    }
    {
        std::vector<Unit> units = { Unit::meter };
        Stops stops(Stops::Offsets(node, units));
        REQUIRE(stops.frames.size() == 0);
        stops.evalVec2(1);
    }
    {
        Stops stops(Stops::FontSize(node));
        REQUIRE(stops.frames.size() == 0);
        stops.evalVec2(1);
    }

}

