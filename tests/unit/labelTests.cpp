#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"
#include "tangram.h"
#include "tile/labels/label.h"

glm::mat4 mvp;
glm::vec2 screen;

TEST_CASE( "Ensure the transition from wait -> sleep when occlusion happens", "[Core][Label]" ) {
    Label l({}, "label", 0, Label::Type::LINE);

    REQUIRE(l.getState() == Label::State::WAIT_OCC);

    l.setOcclusion(true);
    l.update(mvp, screen, 0);

    REQUIRE(l.getState() != Label::State::SLEEP);
    REQUIRE(l.getState() == Label::State::WAIT_OCC);

    l.setOcclusion(true);
    l.occlusionSolved();
    l.update(mvp, screen, 0);

    REQUIRE(l.getState() == Label::State::SLEEP);
}

TEST_CASE( "Ensure the transition from wait -> visible when no occlusion happens", "[Core][Label]" ) {
    Label l({}, "label", 0, Label::Type::LINE);

    REQUIRE(l.getState() == Label::State::WAIT_OCC);

    l.setOcclusion(false);
    l.update(mvp, screen, 0);

    REQUIRE(l.getState() != Label::State::SLEEP);
    REQUIRE(l.getState() == Label::State::WAIT_OCC);

    l.setOcclusion(false);
    l.occlusionSolved();
    l.update(mvp, screen, 0);

    REQUIRE(l.getState() == Label::State::VISIBLE);
}

TEST_CASE( "Ensure the end state of fading out is sleep state", "[Core][Label]" ) {
    Label l({}, "label", 0, Label::Type::LINE);

    l.setOcclusion(false);
    l.occlusionSolved();
    l.update(mvp, screen, 0);
    l.setOcclusion(true);
    l.occlusionSolved();
    l.update(mvp, screen, 0);

    REQUIRE(l.getState() == Label::State::FADING_OUT);

    l.update(mvp, screen, 100);

    REQUIRE(l.getState() == Label::State::SLEEP);
}

