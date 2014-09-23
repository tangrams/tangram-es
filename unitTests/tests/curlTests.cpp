#include "catch/catch.hpp"

#include <iostream>

#include "dataSource/dataSource.h"
#include "glm/glm.hpp"

/*unsigned int Factorial( unsigned int number ) {
      return number > 1 ? Factorial(number-1)*number : 1;
}

TEST_CASE( "Factorials are computed", "[factorial]" ) {
        REQUIRE( Factorial(0) == 1 );
        REQUIRE( Factorial(1) == 1 );
        REQUIRE( Factorial(2) == 2 );
        REQUIRE( Factorial(3) == 6 );
        REQUIRE( Factorial(10) == 3628800 );
}*/

TEST_CASE( "URL Name Check for MapzenVectorTileJson", "[CURL][DataSource][MapzenVectorTileJson]" ) {
    glm::ivec3 tileCoord = glm::ivec3(0,0,0);
    REQUIRE( *constructURL(tileCoord) == "http://vector.mapzen.com/osm/all/0/0/0.json");
    tileCoord = glm::ivec3(19293,24641,16);
    REQUIRE( *constructURL(tileCoord) == "http://vector.mapzen.com/osm/all/16/19293/24641.json");
    tileCoord = glm::ivec3(19293,24641,14);
    REQUIRE( *constructURL(tileCoord) == "http://vector.mapzen.com/osm/all/14/19293/24641.json");
}

TEST_CASE( "Extract tile coordinates from URL check for MapzenVectorTileJson", "[CURL][DataSource][MapzenVectorTileJson]" ) {
    REQUIRE( extractIDFromUrl("http://vector.mapzen.com/osm/all/16/19293/24641.json") == "16_19293_24641" );
    REQUIRE( extractIDFromUrl("http://vector.mapzen.com/osm/all/0/0/0.json") == "0_0_0" );
    REQUIRE( extractIDFromUrl("http://vector.mapzen.com/osm/all/14/19293/24641.json") == "14_19293_24641" );
}

TEST_CASE( "MapzenVectorTileJson::LoadTile check", "[CURL][DataSource][MapzenVectorTileJson]" ) {
    MapzenVectorTileJson dataSource;
    std::vector<glm::ivec3> tileCoords;
    tileCoords.push_back(glm::ivec3(19293,24641,16));
    tileCoords.push_back(glm::ivec3(19293,24641,14));
    tileCoords.push_back(glm::ivec3(0,0,0));
    dataSource.LoadTile(tileCoords);
    //check if all the test tileCoordinates are loaded
    REQUIRE(dataSource.JsonRootSize() == 3);
    //check if all the test tiles have data in the jsonRoots data structure
    REQUIRE(dataSource.CheckDataExists("14_19293_24641") == true);
    REQUIRE(dataSource.CheckDataExists("16_19293_24641") == true);
    REQUIRE(dataSource.CheckDataExists("0_0_0") == true);
    REQUIRE(dataSource.CheckDataExists("0_1_0") == false);
}
