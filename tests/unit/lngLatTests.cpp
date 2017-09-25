#include "catch.hpp"

#include "util/types.h"

using namespace Tangram;

TEST_CASE("LngLat::wrapLongitude correctly wraps to the range (-180.0, 180.0]", "[LngLat]") {

    // Longitudes and latitudes in canonical range remain unchanged.
    CHECK(LngLat::wrapLongitude(0.0) == 0.0);
    CHECK(LngLat::wrapLongitude(37.3) == 37.3);
    CHECK(LngLat::wrapLongitude(180.0) == 180.0);

    // Longitudes above canonical range are wrapped to the canonical range.
    CHECK(LngLat::wrapLongitude(255.1) == -104.9);
    CHECK(LngLat::wrapLongitude(720.0) == 0.0);

    // Longitudes below canonical range are wrapped to the canonical range.
    CHECK(LngLat::wrapLongitude(-255.1) == 104.9);
    CHECK(LngLat::wrapLongitude(-720.0) == 0.0);
    CHECK(LngLat::wrapLongitude(-180.0) == 180.0);

}
