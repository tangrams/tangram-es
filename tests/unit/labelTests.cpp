#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"
#include "tangram.h"
#include "labels/label.h"
#include "labels/textLabel.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define EPSILON 0.00001

using namespace Tangram;

glm::vec2 screenSize(500.f, 500.f);
TextBuffer dummy(nullptr);

TextLabel makeLabel(Label::Transform _transform, Label::Type _type) {
    Label::Options options;
    options.color = 0xff;
    options.offset = {0.0f, 0.0f};
    return TextLabel("label", _transform, _type, {0, 0}, dummy, {0, 0}, options);
}

TEST_CASE( "Ensure the transition from wait -> sleep when occlusion happens", "[Core][Label]" ) {
    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::point));

    REQUIRE(l.getState() == Label::State::wait_occ);
    l.setOcclusion(true);
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);

    REQUIRE(l.getState() != Label::State::sleep);
    REQUIRE(l.getState() == Label::State::wait_occ);
    REQUIRE(l.canOcclude());

    l.setOcclusion(true);
    l.occlusionSolved();
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);

    REQUIRE(l.getState() == Label::State::dead);
    REQUIRE(!l.canOcclude());
}

TEST_CASE( "Ensure the transition from wait -> visible when no occlusion happens", "[Core][Label]" ) {
    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::point));

    REQUIRE(l.getState() == Label::State::wait_occ);

    l.setOcclusion(false);
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);

    REQUIRE(l.getState() != Label::State::sleep);
    REQUIRE(l.getState() == Label::State::wait_occ);

    l.setOcclusion(false);
    l.occlusionSolved();
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);

    REQUIRE(l.getState() == Label::State::fading_in);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 1.f);
    REQUIRE(l.getState() == Label::State::visible);
    REQUIRE(l.canOcclude());
}

TEST_CASE( "Ensure the end state after occlusion is leep state", "[Core][Label]" ) {
    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::point));

    l.setOcclusion(false);
    l.occlusionSolved();
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0.f);

    REQUIRE(l.getState() == Label::State::fading_in);
    REQUIRE(l.canOcclude());

    l.setOcclusion(true);
    l.occlusionSolved();
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0.f);

    REQUIRE(l.getState() == Label::State::sleep);
    REQUIRE(!l.canOcclude());
}

TEST_CASE( "Ensure the out of screen state transition", "[Core][Label]" ) {
    TextLabel l(makeLabel({screenSize*2.f}, Label::Type::point));

    REQUIRE(l.getState() == Label::State::wait_occ);

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0.f);

    REQUIRE(l.getState() == Label::State::out_of_screen);
    REQUIRE(!l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x * 4.f, screenSize.y * 4.f, 0.f, -1.f, 1.f), screenSize, 0.f);
    REQUIRE(l.getState() == Label::State::wait_occ);
    REQUIRE(l.canOcclude());

    l.setOcclusion(false);
    l.occlusionSolved();
    l.update(glm::ortho(0.f, screenSize.x * 4.f, screenSize.y * 4.f, 0.f, -1.f, 1.f), screenSize, 0.f);
    REQUIRE(l.getState() != Label::State::wait_occ);

    REQUIRE(l.getState() == Label::State::fading_in);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x * 4.f, screenSize.y * 4.f, 0.f, -1.f, 1.f), screenSize, 1.f);

    REQUIRE(l.getState() == Label::State::visible);
    REQUIRE(l.canOcclude());
}

TEST_CASE( "Ensure debug labels are always visible and cannot occlude", "[Core][Label]" ) {
    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::debug));

    REQUIRE(l.getState() == Label::State::visible);
    REQUIRE(!l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 1.f);

    REQUIRE(l.getState() == Label::State::visible);
    REQUIRE(!l.canOcclude());
}

TEST_CASE( "Linear interpolation", "[Core][Label][Fade]" ) {
    FadeEffect fadeOut(false, FadeEffect::Interpolation::linear, 1.0);

    REQUIRE(fadeOut.update(0.0) == 1.0);
    REQUIRE(fadeOut.update(0.5) == 0.5);
    REQUIRE(fadeOut.update(0.5) == 0.0);

    fadeOut.update(0.01);

    REQUIRE(fadeOut.isFinished());

    FadeEffect fadeIn(true, FadeEffect::Interpolation::linear, 1.0);

    REQUIRE(fadeIn.update(0.0) == 0.0);
    REQUIRE(fadeIn.update(0.5) == 0.5);
    REQUIRE(fadeIn.update(0.5) == 1.0);

    fadeIn.update(0.01);

    REQUIRE(fadeIn.isFinished());
}

TEST_CASE( "Pow interpolation", "[Core][Label][Fade]" ) {
    FadeEffect fadeOut(false, FadeEffect::Interpolation::pow, 1.0);

    REQUIRE(fadeOut.update(0.0) == 1.0);
    REQUIRE(fadeOut.update(0.5) == 0.75);
    REQUIRE(fadeOut.update(0.5) == 0.0);

    fadeOut.update(0.01);

    REQUIRE(fadeOut.isFinished());

    FadeEffect fadeIn(true, FadeEffect::Interpolation::pow, 1.0);

    REQUIRE(fadeIn.update(0.0) == 0.0);
    REQUIRE(fadeIn.update(0.5) == 0.25);
    REQUIRE(fadeIn.update(0.5) == 1.0);

    fadeIn.update(0.01);

    REQUIRE(fadeIn.isFinished());
}

TEST_CASE( "Sine interpolation", "[Core][Label][Fade]" ) {
    FadeEffect fadeOut(false, FadeEffect::Interpolation::sine, 1.0);

    REQUIRE(std::fabs(fadeOut.update(0.0) - 1.0) < EPSILON);
    REQUIRE(std::fabs(fadeOut.update(1.0) - 0.0) < EPSILON);

    fadeOut.update(0.01);

    REQUIRE(fadeOut.isFinished());

    FadeEffect fadeIn(true, FadeEffect::Interpolation::sine, 1.0);

    REQUIRE(std::fabs(fadeIn.update(0.0) - 0.0) < EPSILON);
    REQUIRE(std::fabs(fadeIn.update(1.0) - 1.0) < EPSILON);

    fadeIn.update(0.01);

    REQUIRE(fadeIn.isFinished());
}

