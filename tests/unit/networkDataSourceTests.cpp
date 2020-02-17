#include "catch.hpp"

#include "data/networkDataSource.h"

using namespace Tangram;

#define TAGS "[NetworkDataSource]"

TEST_CASE("Convert tile coordinates to QuadKey", TAGS) {
    CHECK(NetworkDataSource::tileCoordinatesToQuadKey(TileID(0, 0, 0)) == "");
    CHECK(NetworkDataSource::tileCoordinatesToQuadKey(TileID(0, 0, 1)) == "0");
    CHECK(NetworkDataSource::tileCoordinatesToQuadKey(TileID(0, 0, 10)) == "0000000000");
    CHECK(NetworkDataSource::tileCoordinatesToQuadKey(TileID(3, 5, 3)) == "213");
    CHECK(NetworkDataSource::tileCoordinatesToQuadKey(TileID(5, 3, 4)) == "0123");
}

TEST_CASE("Build URL for tile from template", TAGS) {

    SECTION("Template with x/y/z") {
        std::string url = "https://some.domain/tiles/{z}/{x}/{y}.json";
        NetworkDataSource::UrlOptions urlOptions;

        CHECK(NetworkDataSource::buildUrlForTile(TileID(0, 1, 2), url, urlOptions, 0) == "https://some.domain/tiles/2/0/1.json");
        CHECK(NetworkDataSource::buildUrlForTile(TileID(12345, 54321, 20), url, urlOptions, 0) == "https://some.domain/tiles/20/12345/54321.json");
    }

    SECTION("Template with x/y/z in TMS mode") {
        std::string url = "https://some.domain/tiles/{z}/{x}/{y}.json";
        NetworkDataSource::UrlOptions urlOptions;
        urlOptions.isTms = true;

        CHECK(NetworkDataSource::buildUrlForTile(TileID(0, 1, 2), url, urlOptions, 0) == "https://some.domain/tiles/2/0/2.json");
        CHECK(NetworkDataSource::buildUrlForTile(TileID(12345, 54321, 20), url, urlOptions, 0) == "https://some.domain/tiles/20/12345/994254.json");
    }

    SECTION("Template with subdomains") {
        std::string url = "https://{s}.some.domain/tiles/{z}/{x}/{y}.json";
        NetworkDataSource::UrlOptions urlOptions;
        urlOptions.subdomains = { "zero", "one" };

        CHECK(NetworkDataSource::buildUrlForTile(TileID(0, 1, 2), url, urlOptions, 0) == "https://zero.some.domain/tiles/2/0/1.json");
        CHECK(NetworkDataSource::buildUrlForTile(TileID(0, 1, 2), url, urlOptions, 1) == "https://one.some.domain/tiles/2/0/1.json");
    }

    SECTION("Template with quadkey") {
        std::string url = "file://tiles/{q}.blah";
        NetworkDataSource::UrlOptions urlOptions;

        CHECK(NetworkDataSource::buildUrlForTile(TileID(0, 0, 1), url, urlOptions, 0) == "file://tiles/0.blah");
        CHECK(NetworkDataSource::buildUrlForTile(TileID(3, 5, 3), url, urlOptions, 0) == "file://tiles/213.blah");
    }
}
