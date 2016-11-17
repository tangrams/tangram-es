#include "catch.hpp"
#include "tangram.h"
#include "labels/label.h"
#include "view/view.h"
#include "style/textStyle.h"
#include "labels/screenTransform.h"
#include "labels/textLabel.h"
#include "labels/textLabels.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define EPSILON 0.00001

using namespace Tangram;

glm::vec2 screenSize(500.f, 500.f);
TextStyle dummyStyle("textStyle", nullptr);
TextLabels dummy(dummyStyle);
Label::AABB bounds(0.f, 0.f, 500.f, 500.f);

struct TestTransform {
    ScreenTransform::Buffer buffer;
    Range range;
    ScreenTransform transform;
    TestTransform() : transform(buffer, range) {}
};

TextLabel makeLabel(glm::vec2 _transform, Label::Type _type) {
    Label::Options options;
    options.offset = {0.0f, 0.0f};
    options.anchors.anchor[0] = LabelProperty::Anchor::center;
    options.anchors.count = 1;
    options.showTransition.time = 0.2;
    options.hideTransition.time = 0.2;

    TextRange textRanges;

    return TextLabel({{glm::vec3(_transform, 0)}}, _type, options,
            {}, {0, 0}, dummy, textRanges,
            TextLabelProperty::Align::none);
}

View makeView() {
    View view(256, 256);

    view.setPosition(0, 0);
    view.setZoom(0);
    view.update(false);

    return view;
}

TEST_CASE( "Ensure the transition from wait -> sleep when occlusion happens", "[Core][Label]" ) {
    View view = makeView();
    TestTransform t1, t2;

    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::point));

    REQUIRE(l.state() == Label::State::none);
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), view.state(), &bounds, t1.transform);

    REQUIRE(l.state() != Label::State::sleep);
    REQUIRE(l.state() == Label::State::none);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), view.state(), &bounds, t2.transform);
    l.occlude(true);
    l.evalState(0);

    REQUIRE(l.state() == Label::State::sleep);
}

TEST_CASE( "Ensure the transition from wait -> visible when no occlusion happens", "[Core][Label]" ) {
    View view = makeView();
    TestTransform t1, t2;

    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::point));

    REQUIRE(l.state() == Label::State::none);

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), view.state(), &bounds, t1.transform);
    l.occlude(false);
    l.evalState(0);

    REQUIRE(l.state() == Label::State::fading_in);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), view.state(), &bounds, t2.transform);
    l.evalState(1.f);

    REQUIRE(l.state() == Label::State::visible);
    REQUIRE(l.canOcclude());
}

TEST_CASE( "Ensure the end state after occlusion is leep state", "[Core][Label]" ) {
    View view = makeView();
    TestTransform t1, t2;

    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::point));

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), view.state(), &bounds, t1.transform);
    l.occlude(false);
    l.evalState(0);

    REQUIRE(l.state() == Label::State::fading_in);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), view.state(), &bounds, t2.transform);
    l.occlude(true);
    l.evalState(1.f);

    REQUIRE(l.state() == Label::State::sleep);
    REQUIRE(l.canOcclude());
}

TEST_CASE( "Ensure the sleep transition for out of screen labels", "[Core][Label]" ) {
    View view = makeView();
    TestTransform t1, t2;

    TextLabel l(makeLabel({screenSize*4.f}, Label::Type::point));

    REQUIRE(l.state() == Label::State::none);

    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), view.state(), &bounds, t1.transform);

    REQUIRE(l.state() == Label::State::sleep);
    REQUIRE(l.canOcclude());

    l.update(glm::ortho(0.f, screenSize.x * 4.f, screenSize.y * 4.f, 0.f, -1.f, 1.f), view.state(), &bounds, t2.transform);
    l.evalState(0);
    REQUIRE(l.state() != Label::State::none);

    REQUIRE(l.state() == Label::State::fading_in);
    REQUIRE(l.canOcclude());
}

TEST_CASE( "Ensure debug labels are always visible and cannot occlude", "[Core][Label]" ) {
    View view = makeView();

    TextLabel l(makeLabel({screenSize/2.f}, Label::Type::debug));

    REQUIRE(l.state() == Label::State::visible);
    REQUIRE(!l.canOcclude());

    TestTransform t1;
    l.update(glm::ortho(0.f, screenSize.x, screenSize.y, 0.f, -1.f, 1.f), view.state(), &bounds, t1.transform);
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
