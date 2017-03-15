#include "catch.hpp"

#include <iostream>

/*#include "dataSource.h"
#include "mapTile.h"
#include "glm/glm.hpp"
#include "tileID.h"*/


TEST_CASE( "URL Name Check for MapzenVectorTileJson", "[CURL][DataSource][MapzenVectorTileJson]" ) {
    /*MapzenVectorTileJson dataSource;
    TileID tileCoord = TileID(0,0,0);
    std::string url;
    *dataSource.constructURL(tileCoord, url);
    REQUIRE( url == "http://tile.mapzen.com/mapzen/vector/v1/all/0/0/0.json");
    TileID tileCoord2 = TileID(19293,24641,16);
    *dataSource.constructURL(tileCoord2, url);
    REQUIRE( url == "http://tile.mapzen.com/mapzen/vector/v1/all/16/19293/24641.json");
    TileID tileCoord3 = TileID(19293,24641,14);
    *dataSource.constructURL(tileCoord3, url);
    REQUIRE( url == "http://tile.mapzen.com/mapzen/vector/v1/all/14/19293/24641.json");*/
}

TEST_CASE( "Extract tile coordinates from URL check for MapzenVectorTileJson", "[CURL][DataSource][MapzenVectorTileJson]" ) {
    /*MapzenVectorTileJson dataSource;
    REQUIRE( dataSource.extractIDFromUrl("http://tile.mapzen.com/mapzen/vector/v1/all/16/19293/24641.json") == TileID(19293, 24641, 16) );
    REQUIRE( dataSource.extractIDFromUrl("http://tile.mapzen.com/mapzen/vector/v1/all/0/0/0.json") == TileID(0,0,0) );
    REQUIRE( dataSource.extractIDFromUrl("http://tile.mapzen.com/mapzen/vector/v1/all/14/19293/24641.json") == TileID(19293, 24641, 14) );*/
}

TEST_CASE( "MapzenVectorTileJson::LoadTile check", "[CURL][DataSource][MapzenVectorTileJson]" ) {
    /*MapzenVectorTileJson dataSource;
    std::vector<TileID> tileCoords;
    tileCoords.push_back(TileID(19293,24641,16));
    tileCoords.push_back(TileID(19293,24641,14));
    tileCoords.push_back(TileID(0,0,0));
    dataSource.loadTile(tileCoords);
    //check if all the test tileCoordinates are loaded
    REQUIRE(dataSource.JsonRootSize() == 3);
    //check if all the test tiles have data in the jsonRoots data structure
    REQUIRE(dataSource.CheckDataExists(TileID(19293, 24641, 14)) == true);
    REQUIRE(dataSource.CheckDataExists(TileID(19293, 24641, 16)) == true);
    REQUIRE(dataSource.CheckDataExists(TileID(0, 0, 0)) == true);
    REQUIRE(dataSource.CheckDataExists(TileID(0, 1, 0)) == false);*/
}
