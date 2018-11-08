#include "catch.hpp"

#include "view/flyTo.h"
#include "view/view.h"
#include "log.h"

using namespace Tangram;

const static double nearZero = 0.00000001;

View getView() {
    View view(1000, 1000);
    view.setPosition(0, 0);
    view.setZoom(0);
    view.update(false);
    return view;
}

TEST_CASE( "FlyTo Zoom", "[View][FlyTo]" ) {

    auto view = getView();

    glm::dvec3 start(0.0, 0.0, 10.0);
    glm::dvec3 end1(0.0, 0.0, 20.0);

    double dist1 = 0.0;

    auto fn1 = getFlyToFunction(view, start, end1, dist1);

    REQUIRE(end1 == fn1(1.0));
    // Linear interpolation of zoom
    REQUIRE(std::abs((start + (end1 - start) * 0.5).z - fn1(0.5).z) < nearZero);

    /// Test with Minimal movement
    glm::dvec3 end2(0.0, 0.0000000001, 20.0);
    double dist2 = 0.0;

    auto fn2 = getFlyToFunction(view, start, end2, dist2);

    REQUIRE(start == fn2(0.0));
    REQUIRE(end2 == fn2(1.0));

    // Linear interpolation of zoom
    REQUIRE(std::abs((start + (end2 - start) * 0.5).z - fn2(0.5).z) < nearZero);

}
TEST_CASE( "FlyTo with equal start and end", "[View][FlyTo]" ) {

    auto view = getView();

    glm::dvec3 start(10.0, -10.0, 10.0);
    glm::dvec3 end(10.0, -10.0, 10.0);

    double dist = 0.0;

    auto fn = getFlyToFunction(view, start, end, dist);

    REQUIRE(dist == 0.0);
    REQUIRE(start == fn(0.0));
    REQUIRE(end == fn(1.0));
}

TEST_CASE( "FlyTo Bremen to New York", "[View][FlyTo]" ) {

    auto view = getView();

    glm::dvec2 a = MapProjection::lngLatToProjectedMeters({8.820, 53.080});
    glm::dvec2 b = MapProjection::lngLatToProjectedMeters({-74.009, 40.705});

    glm::dvec3 start(a.x, a.y, 16.0);
    glm::dvec3 end(b.x, b.y, 16.0);

    double dist = 0.0;

    auto fn = getFlyToFunction(view, start, end, dist);

    // LOG("%f %f",  fn(0.0).x,  fn(0.0).y);
    // LOG("%f %f",  start.x,  start.y);

    auto center1 = glm::dvec2(start + (end - start) * 0.5);
    auto center2 = fn(0.5);

    REQUIRE(std::abs(center1.x - center2.x) < nearZero);
    REQUIRE(std::abs(center1.y - center2.y) < nearZero);

    REQUIRE(center2.z < start.z);

}
