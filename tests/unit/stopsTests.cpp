#include "catch.hpp"

#include "scene/stops.h"
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

    REQUIRE(stops.evalExpFloat(-3) == 0.f);
    REQUIRE(stops.evalExpFloat(0) == 0.f);
    REQUIRE(std::abs(stops.evalExpFloat(0.3) - 2.31144f) < 0.00001);
    REQUIRE(stops.evalExpFloat(1) == 10.f);
    REQUIRE(stops.evalExpFloat(3) == 18.f);
    REQUIRE(stops.evalExpFloat(5) == 50.f);
    REQUIRE(std::abs(stops.evalExpFloat(6) - 33.33333f) < 0.00001);
    REQUIRE(stops.evalExpFloat(7) == 0.f);
    REQUIRE(stops.evalExpFloat(8) == 3.f);
    REQUIRE(stops.evalExpFloat(8.4) == 3.f); // flat interpolation
    REQUIRE(stops.evalExpFloat(9.3) == 3.f); // flat interpolation
    REQUIRE(stops.evalExpFloat(10) == 3.f);

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

    Stops stops(Stops::Widths(node, proj, {}));

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
        Stops stops(Stops::Widths(node, proj, units));
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

TEST_CASE("2 dimension stops for icon sizes with mixed units", "[Stops][YAML]") {
    YAML::Node node = YAML::Load(R"END(
        [[6, [18.0 px, 14px]], [13, [20 m, 15px]], [16, [24, 18]]]
    )END");

    Stops stops(Stops::Sizes(node, StyleParam::unitsForStyleParam(StyleParamKey::size)));

    REQUIRE(stops.frames.size() == 3);

    REQUIRE(stops.evalSize(0).get<glm::vec2>() == glm::vec2(18, 14));
    REQUIRE(stops.evalSize(13).get<glm::vec2>() == glm::vec2(20, 15));
    REQUIRE(stops.evalSize(18).get<glm::vec2>() == glm::vec2(24, 18));
}


TEST_CASE("1 dimension stops for icon sizes", "[Stops][YAML]") {
    YAML::Node node = YAML::Load(R"END(
        [[6, 18], [13, 20]]
    )END");

    Stops stops(Stops::Sizes(node, StyleParam::unitsForStyleParam(StyleParamKey::size)));

    REQUIRE(stops.frames.size() == 2);

    REQUIRE(stops.evalSize(0).get<float>() == 18);
    REQUIRE(stops.evalSize(18).get<float>() == 20);
}

