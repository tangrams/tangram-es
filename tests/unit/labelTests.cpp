#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"
#include "tangram.h"
#include "tile/labels/label.h"

#define EPSILON 0.00001

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

TEST_CASE( "Linear interpolation", "[Core][Label][Fade]" ) {
    FadeEffect fadeOut(false, FadeEffect::Interpolation::LINEAR, 1.0);

    REQUIRE(fadeOut.update(0.0) == 1.0);
    REQUIRE(fadeOut.update(0.5) == 0.5);
    REQUIRE(fadeOut.update(0.5) == 0.0);

    fadeOut.update(0.01);

    REQUIRE(fadeOut.isFinished());

    FadeEffect fadeIn(true, FadeEffect::Interpolation::LINEAR, 1.0);

    REQUIRE(fadeIn.update(0.0) == 0.0);
    REQUIRE(fadeIn.update(0.5) == 0.5);
    REQUIRE(fadeIn.update(0.5) == 1.0);

    fadeIn.update(0.01);

    REQUIRE(fadeIn.isFinished());
}

TEST_CASE( "Pow interpolation", "[Core][Label][Fade]" ) {
    FadeEffect fadeOut(false, FadeEffect::Interpolation::POW, 1.0);

    REQUIRE(fadeOut.update(0.0) == 1.0);
    REQUIRE(fadeOut.update(0.5) == 0.75);
    REQUIRE(fadeOut.update(0.5) == 0.0);

    fadeOut.update(0.01);

    REQUIRE(fadeOut.isFinished());

    FadeEffect fadeIn(true, FadeEffect::Interpolation::POW, 1.0);

    REQUIRE(fadeIn.update(0.0) == 0.0);
    REQUIRE(fadeIn.update(0.5) == 0.25);
    REQUIRE(fadeIn.update(0.5) == 1.0);

    fadeIn.update(0.01);

    REQUIRE(fadeIn.isFinished());
}

TEST_CASE( "Sine interpolation", "[Core][Label][Fade]" ) {
    FadeEffect fadeOut(false, FadeEffect::Interpolation::SINE, 1.0);

    REQUIRE(abs(fadeOut.update(0.0) - 1.0) < EPSILON);
    REQUIRE(abs(fadeOut.update(1.0) - 0.0) < EPSILON);

    fadeOut.update(0.01);

    REQUIRE(fadeOut.isFinished());

    FadeEffect fadeIn(true, FadeEffect::Interpolation::SINE, 1.0);

    REQUIRE(abs(fadeIn.update(0.0) - 0.0) < EPSILON);
    REQUIRE(abs(fadeIn.update(1.0) - 1.0) < EPSILON);

    fadeIn.update(0.01);

    REQUIRE(fadeIn.isFinished());
}

