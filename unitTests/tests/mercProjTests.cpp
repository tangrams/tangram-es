#include "catch/catch.hpp"

#include <iostream>
#include <iomanip>

#include "glm/glm.hpp"
#include "util/projection.h"

#include <stdio.h>


TEST_CASE( "Testing some functionality for mercator projection", "[MERCATOR][PROJECTION]" ) {
    MercProjection mercProjection = MercProjection();
    glm::dvec2 latLon = glm::dvec2(0.0,0.0);
    glm::dvec2 worldCoord = glm::dvec2(0.0, 0.0);
    glm::ivec3 tileCoord = glm::ivec3(2, 2, 3);
    //check if all the test tileCoordinates are loaded
    /*REQUIRE(dataSource.JsonRootSize() == 3);
    //check if all the test tiles have data in the jsonRoots data structure
    REQUIRE(dataSource.CheckDataExists("14_19293_24641") == true);
    REQUIRE(dataSource.CheckDataExists("16_19293_24641") == true);
    REQUIRE(dataSource.CheckDataExists("0_0_0") == true);
    REQUIRE(dataSource.CheckDataExists("0_1_0") == false);*/
    glm::dvec2 testMeters = mercProjection.LatLonToMeters(latLon);
    glm::dvec2 testMeters2 = mercProjection.MetersToLatLon(worldCoord);
    printf("%f\t%f", testMeters.x, testMeters.y);
}
