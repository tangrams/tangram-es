#include "catch.hpp"
#include "glm/glm.hpp"
#include "util/mapProjection.h"

using namespace Tangram;

TEST_CASE("MapProjection correctly converts between LngLat and ProjectedMeters", "[projection]") {

    double epsilonDegrees = 0.00001;
    double epsilonMeters = 0.01;

    struct TestPair {
        LngLat lngLat;
        ProjectedMeters meters;
    };

    // Pairs of LngLat and ProjectedMeters that should be equivalent when converted.
    auto pair = GENERATE(
            TestPair{LngLat(0.0, 0.0), ProjectedMeters(0.0, 0.0)},
            TestPair{LngLat(180.0, 0.0), ProjectedMeters(MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS, 0.0)},
            TestPair{LngLat(-180.0, 0.0), ProjectedMeters(-MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS, 0.0)},
            TestPair{LngLat(0.0, MapProjection::MAX_LATITUDE_DEGREES), ProjectedMeters(0.0, MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS)},
            TestPair{LngLat(0.0, -MapProjection::MAX_LATITUDE_DEGREES), ProjectedMeters(0.0, -MapProjection::EARTH_HALF_CIRCUMFERENCE_METERS)}
            );

    auto convertedMeters = MapProjection::lngLatToProjectedMeters(pair.lngLat);
    CHECK_THAT(convertedMeters.x, Catch::WithinAbs(pair.meters.x, epsilonMeters));
    CHECK_THAT(convertedMeters.y, Catch::WithinAbs(pair.meters.y, epsilonMeters));

    auto convertedLngLat = MapProjection::projectedMetersToLngLat(pair.meters);
    CHECK_THAT(convertedLngLat.longitude, Catch::WithinAbs(pair.lngLat.longitude, epsilonDegrees));
    CHECK_THAT(convertedLngLat.latitude, Catch::WithinAbs(pair.lngLat.latitude, epsilonDegrees));

}

TEST_CASE("MapProjection correctly converts between zoom and metersPerPixel", "[projection]") {

    double epsilon = 0.00001;

    SECTION("zoom 0") {
        auto metersPerPixel = MapProjection::metersPerPixelAtZoom(0.0);
        auto expected = MapProjection::EARTH_CIRCUMFERENCE_METERS / MapProjection::tileSize();
        CHECK_THAT(metersPerPixel, Catch::WithinAbs(expected, epsilon));
    }

    SECTION("zoom n + 1") {
        auto zoom = GENERATE(0.0, 7.0, 16.0, 24.0);

        auto metersPerPixel = MapProjection::metersPerPixelAtZoom(zoom);
        auto metersPerPixelHalved = metersPerPixel * 0.5;
        auto metersPerPixelAtNextZoom = MapProjection::metersPerPixelAtZoom(zoom + 1.0);
        auto convertedZoom = MapProjection::zoomAtMetersPerPixel(metersPerPixel);

        CHECK_THAT(convertedZoom, Catch::WithinAbs(zoom, epsilon));
        CHECK_THAT(metersPerPixelAtNextZoom, Catch::WithinAbs(metersPerPixelHalved, epsilon));
    }

}
