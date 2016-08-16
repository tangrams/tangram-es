#include "catch.hpp"
#include "tangram.h"
#include "labels/label.h"
#include "style/textStyle.h"
#include "labels/textLabel.h"
#include "labels/textLabels.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define EPSILON 0.00001

using namespace Tangram;

glm::vec2 screenSize(500.f, 500.f);
TextStyle dummyStyle("textStyle", nullptr);
TextLabels dummy(dummyStyle);

TextLabel makeLabel(Label::Transform _transform, Label::Type _type) {
    Label::Options options;
    options.offset = {0.0f, 0.0f};
    options.anchors.anchor[0] = LabelProperty::Anchor::center;
    options.anchors.count = 1;
    options.showTransition.time = 0.2;
    options.hideTransition.time = 0.2;

    TextRange textRanges;

    return TextLabel(_transform, _type, options,
            {}, {0, 0}, dummy, textRanges,
            TextLabelProperty::Align::none);
}

TEST_CASE( "Ensure the transition from wait -> sleep when occlusion happens", "[Core][Label]" ) {
    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::point));

    REQUIRE(l.state() == Label::State::none);
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);

    REQUIRE(l.state() != Label::State::sleep);
    REQUIRE(l.state() == Label::State::none);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
    l.occlude(true);
    l.evalState(0);

    REQUIRE(l.state() == Label::State::sleep);
}

TEST_CASE( "Ensure the transition from wait -> visible when no occlusion happens", "[Core][Label]" ) {
    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::point));

    REQUIRE(l.state() == Label::State::none);

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
    l.occlude(false);
    l.evalState(0);

    REQUIRE(l.state() == Label::State::fading_in);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
    l.evalState(1.f);

    REQUIRE(l.state() == Label::State::visible);
    REQUIRE(l.canOcclude());
}

TEST_CASE( "Ensure the end state after occlusion is leep state", "[Core][Label]" ) {
    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::point));

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
    l.occlude(false);
    l.evalState(0);

    REQUIRE(l.state() == Label::State::fading_in);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
    l.occlude(true);
    l.evalState(1.f);

    // Depends whether fading-in labels fade out or set to sleep in evalState
    // REQUIRE(l.state() == Label::State::fading_out);
    // l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
    // l.occlude(true);
    // l.evalState(screenSize, 1.f);

    REQUIRE(l.state() == Label::State::sleep);
    REQUIRE(l.canOcclude());
}

TEST_CASE( "Ensure the out of screen state transition", "[Core][Label]" ) {
    TextLabel l(makeLabel({screenSize*2.f}, Label::Type::point));

    REQUIRE(l.state() == Label::State::none);

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);

    REQUIRE(l.state() == Label::State::out_of_screen);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x * 4.f, screenSize.y * 4.f, 0.f, -1.f, 1.f), screenSize, 0);
    l.evalState(0);
    REQUIRE(l.state() != Label::State::none);

    REQUIRE(l.state() == Label::State::fading_in);
    REQUIRE(l.canOcclude());
}

TEST_CASE( "Ensure debug labels are always visible and cannot occlude", "[Core][Label]" ) {
    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::debug));

    REQUIRE(l.state() == Label::State::visible);
    REQUIRE(!l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
    l.evalState(1.f);

    REQUIRE(l.state() == Label::State::visible);
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

#if 0
TEST_CASE( "Ensure anchor fallback behavior on first fallback", "[Core][Label]" ) {
    TextLabel l = makeLabelWithAnchorFallbacks();

    REQUIRE(l.state() == Label::State::none);

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
    l.occlude(true);
    l.evalState(1.f);

    REQUIRE(l.state() == Label::State::anchor_fallback);
}

TEST_CASE( "Ensure anchor fallback behavior when looping over all fallbacks without finding one", "[Core][Label]" ) {
    TextLabel l = makeLabelWithAnchorFallbacks();

    // none -> anchor_fallback
    {
        REQUIRE(l.state() == Label::State::none);

        l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
        l.occlude(true);
        l.evalState(1.f);

        REQUIRE(l.state() == Label::State::anchor_fallback);
    }

    // anchor_fallback -> anchor_fallback (move to fourth anchor)
    {
        for (int i = 0; i < 3; ++i) {
            l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
            l.occlude(true);
            l.evalState(1.f);

            REQUIRE(l.state() == Label::State::anchor_fallback);
        }

        REQUIRE(l.anchorType() == LabelProperty::Anchor::top);
    }

    // anchor_fallback -> fading_out (all anchor tested)
    {
        l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
        l.occlude(true);
        l.evalState(1.f);
        REQUIRE(l.state() == Label::State::sleep);

        l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
        l.occlude(true);
        l.evalState(1.f);
        REQUIRE(l.state() == Label::State::sleep);
    }
}

TEST_CASE( "Ensure anchor fallback behavior when looping over all fallback and finding one", "[Core][Label]" ) {
    TextLabel l = makeLabelWithAnchorFallbacks();

    REQUIRE(l.state() == Label::State::none);

    // move to third anchor
    for (int i = 0; i < 3; ++i) {
        l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
        l.occlude(true);
        l.evalState(1.f);

        REQUIRE(l.state() == Label::State::anchor_fallback);
    }

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), screenSize, 0);
    l.occlude(false);
    l.evalState(1.f);

    REQUIRE(l.anchorType() == LabelProperty::Anchor::left);
    REQUIRE(l.state() == Label::State::visible);
}
#endif
