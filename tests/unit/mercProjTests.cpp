#include "catch.hpp"

#include <iostream>
#include <iomanip>

#include "glm/glm.hpp"
#include "util/mapProjection.h"

#include <stdio.h>

using namespace Tangram;

TEST_CASE( "Testing some functionality for mercator projection", "[MERCATOR][PROJECTION]" ) {
    MercatorProjection mercProjection = MercatorProjection();
    glm::dvec2 lonLat = glm::dvec2(0.0,0.0);
    glm::dvec2 worldCoord = glm::dvec2(0.0, 0.0);
    //glm::ivec3 tileCoord = glm::ivec3(2, 2, 3);
    double epsilon = 0.000000000000000001;
    //check if all the test tileCoordinates are loaded
    /*REQUIRE(dataSource.JsonRootSize() == 3);
    //check if all the test tiles have data in the jsonRoots data structure
    REQUIRE(dataSource.CheckDataExists("14_19293_24641") == true);
    REQUIRE(dataSource.CheckDataExists("16_19293_24641") == true);
    REQUIRE(dataSource.CheckDataExists("0_0_0") == true);
    REQUIRE(dataSource.CheckDataExists("0_1_0") == false);*/
    glm::dvec2 testMeters = mercProjection.LonLatToMeters(lonLat);
    glm::dvec2 testLonLat = mercProjection.MetersToLonLat(worldCoord);
    REQUIRE( (testMeters.x - worldCoord.x) < epsilon);
    REQUIRE( (testMeters.y - worldCoord.y) < epsilon);
    REQUIRE( (testLonLat.x - lonLat.x) < epsilon);
    REQUIRE( (testLonLat.y - lonLat.y) < epsilon);
}
