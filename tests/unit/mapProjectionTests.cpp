#include "catch.hpp"
#include "approxVec.h"
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
    CHECK_THAT(convertedMeters, IsApproxEqualToVec<glm::dvec2>(pair.meters, epsilonMeters));

    auto convertedLngLat = MapProjection::projectedMetersToLngLat(pair.meters);
    CHECK_THAT(convertedLngLat.longitude, Catch::WithinAbs(pair.lngLat.longitude, epsilonDegrees));
    CHECK_THAT(convertedLngLat.latitude, Catch::WithinAbs(pair.lngLat.latitude, epsilonDegrees));

}
