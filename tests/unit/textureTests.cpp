#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "gl/texture.h"

using namespace Tangram;

class TestTexture : public Texture {
public:
    using Texture::Texture;
    const std::vector<DirtyRange>& dirtyRanges() { return m_dirtyRanges; }
};

TEST_CASE("Merging of dirty Regions - Non overlapping, test ordering", "[Texture]") {
    TestTexture texture(512, 512);
    REQUIRE(texture.dirtyRanges().size() == 0);

    // A range from 20-30
    texture.setDirty(20, 10);
    REQUIRE(texture.dirtyRanges().size() == 1);

    // B range from 0-10
    texture.setDirty(0, 10);
    REQUIRE(texture.dirtyRanges().size() == 2);

    // C range from 40-60
    texture.setDirty(40, 20);
    REQUIRE(texture.dirtyRanges().size() == 3);

    // B
    REQUIRE(texture.dirtyRanges()[0].min == 0);
    REQUIRE(texture.dirtyRanges()[0].max == 10);

    // A
    REQUIRE(texture.dirtyRanges()[1].min == 20);
    REQUIRE(texture.dirtyRanges()[1].max == 30);

    // C
    REQUIRE(texture.dirtyRanges()[2].min == 40);
    REQUIRE(texture.dirtyRanges()[2].max == 60);

}

TEST_CASE("Merging of dirty Regions - Merge overlapping", "[Texture]") {
    TestTexture texture(512, 512);
    REQUIRE(texture.dirtyRanges().size() == 0);

    // range from 50-100
    texture.setDirty(50, 50);
    REQUIRE(texture.dirtyRanges().size() == 1);

    // range from 20-70
    texture.setDirty(20, 50);
    REQUIRE(texture.dirtyRanges().size() == 1);

    REQUIRE(texture.dirtyRanges()[0].min == 20);
    REQUIRE(texture.dirtyRanges()[0].max == 100);
}

TEST_CASE("Merging of dirty Regions - Merge three regions, when 3rd region is added", "[Texture]") {
    { // just touching

        TestTexture texture(512, 512);
        REQUIRE(texture.dirtyRanges().size() == 0);

        // range from 50-100
        texture.setDirty(50, 50);
        REQUIRE(texture.dirtyRanges().size() == 1);
        REQUIRE(texture.dirtyRanges()[0].min == 50);
        REQUIRE(texture.dirtyRanges()[0].max == 100);

        // range from 200-250
        texture.setDirty(200, 50);
        REQUIRE(texture.dirtyRanges().size() == 2);
        REQUIRE(texture.dirtyRanges()[1].min == 200);
        REQUIRE(texture.dirtyRanges()[1].max == 250);

        // range from 100-200
        texture.setDirty(100, 100);
        REQUIRE(texture.dirtyRanges().size() == 1);
        REQUIRE(texture.dirtyRanges()[0].min == 50);
        REQUIRE(texture.dirtyRanges()[0].max == 250);
    }

    { // overlapping

        TestTexture texture(512, 512);
        REQUIRE(texture.dirtyRanges().size() == 0);

        // range from 50-150
        texture.setDirty(50, 100);
        REQUIRE(texture.dirtyRanges().size() == 1);
        REQUIRE(texture.dirtyRanges()[0].min == 50);
        REQUIRE(texture.dirtyRanges()[0].max == 150);

        // range from 200-250
        texture.setDirty(200, 50);
        REQUIRE(texture.dirtyRanges().size() == 2);
        REQUIRE(texture.dirtyRanges()[1].min == 200);
        REQUIRE(texture.dirtyRanges()[1].max == 250);

        // range from 300-350
        texture.setDirty(300, 50);
        REQUIRE(texture.dirtyRanges().size() == 3);
        REQUIRE(texture.dirtyRanges()[2].min == 300);
        REQUIRE(texture.dirtyRanges()[2].max == 350);

        // range from 100-300
        texture.setDirty(100, 200);
        REQUIRE(texture.dirtyRanges().size() == 1);
        REQUIRE(texture.dirtyRanges()[0].min == 50);
        REQUIRE(texture.dirtyRanges()[0].max == 350);
    }

}
