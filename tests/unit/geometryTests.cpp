#include "catch.hpp"

#include "data/geometry.h"

using namespace Tangram;

TEST_CASE( "Test points", "[Core][Geometry]" ) {
    Geometry<int> geom;

    for (int i = 0; i < 4; i++) {
        geom.addPoint(i);
    }

    REQUIRE(geom.points().size() == 4);
    REQUIRE(geom.lines().size() == 0);
    REQUIRE(geom.polygons().size() == 0);

    int i = 0;
    for (auto& p : geom.points()) {
        REQUIRE(p == i++);
    }
}

TEST_CASE( "Test lines", "[Core][Geometry]" ) {
    Geometry<int> geom;

    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 4; i++) {
            geom.addPoint((j * 4) + i);
        }
        geom.endLine();
    }
    REQUIRE(geom.points().size() == 2 * 4);
    REQUIRE(geom.lines().size() == 2);
    REQUIRE(geom.polygons().size() == 0);

    int i = 0;
    for (auto& line : geom.lines()) {
        for (auto& p : line) {
            REQUIRE(p == i++);
        }
    }
}

TEST_CASE( "Test polygons", "[Core][Geometry]" ) {
    Geometry<int> geom;

    for (int p = 0; p < 2; p++) {
        for (int l = 0; l < 3; l++) {
            for (int i = 0; i < 4; i++) {
                geom.addPoint((p * 3 * 4) + (l * 4) + i);
            }
            geom.endRing();
        }
        geom.endPoly();
    }

    REQUIRE(geom.points().size() == 2 * 3 * 4);
    REQUIRE(geom.lines().size() == 2 * 3);
    REQUIRE(geom.polygons().size() == 2);

    int i = 0;
    for (auto& poly : geom.polygons()) {
        for (auto& line : poly) {
            for (auto& p : line) {
                REQUIRE(p == i++);
            }
        }
    }
}
