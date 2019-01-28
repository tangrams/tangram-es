#include "catch.hpp"

#include "scene/styleParam.h"

#include "glm/vec3.hpp"
#include "yaml-cpp/yaml.h"

using namespace Tangram;

TEST_CASE("Correctly parse valid ValueUnitPair", "[StyleParam]") {

    float epsilon = 1e-6f;

    StyleParam::ValueUnitPair result;

    struct ValueUnitTestPair {
        std::string input;
        StyleParam::ValueUnitPair expected;
    };

    auto testPair = GENERATE(
            ValueUnitTestPair{"2px", {2.f, Unit::pixel}},
            ValueUnitTestPair{"3.14s", {3.14f, Unit::seconds}},
            ValueUnitTestPair{"14 ms", {14.f, Unit::milliseconds}},
            ValueUnitTestPair{"99.99    m", {99.99f, Unit::meter}},
            ValueUnitTestPair{"55%", {55.f, Unit::percentage}},
            ValueUnitTestPair{"404", {404.f, Unit::none}}
            );

    DYNAMIC_SECTION("Input string: " << testPair.input) {
        CHECK(StyleParam::parseValueUnitPair(testPair.input, result));
        CHECK(result.unit == testPair.expected.unit);
        CHECK_THAT(result.value, Catch::WithinAbs(testPair.expected.value, epsilon));
    }
}

TEST_CASE("Correctly reject invalid ValueUnitPair", "[StyleParam]") {

    StyleParam::ValueUnitPair result;

    auto invalid = GENERATE(
            std::string(""),
            std::string("null"),
            std::string("px"),
            std::string("m22")
            );

    DYNAMIC_SECTION("Input string: " << invalid) {
        CHECK_FALSE(StyleParam::parseValueUnitPair(invalid, result));
    }
}

TEST_CASE("Correctly parse valid Vec3", "[StyleParam]") {

    float epsilon = 1e-6f;

    UnitVec<glm::vec3> result;

    auto input = YAML::Load("[0m, 40px, 8]");

    REQUIRE(input.IsSequence());
    CHECK(StyleParam::parseVec3(input, UnitSet{Unit::none, Unit::meter, Unit::pixel}, result));
    CHECK(result.units[0] == Unit::meter);
    CHECK(result.units[1] == Unit::pixel);
    CHECK(result.units[2] == Unit::none);
    CHECK_THAT(result.value[0], Catch::WithinAbs(0.f, epsilon));
    CHECK_THAT(result.value[1], Catch::WithinAbs(40.f, epsilon));
    CHECK_THAT(result.value[2], Catch::WithinAbs(8.f, epsilon));

}

TEST_CASE("Correctly reject invalid Vec3", "[StyleParam]") {

    UnitVec<glm::vec3> result;

    SECTION("Not enough elements") {
        auto input = YAML::Load("[0m, 40px]");
        CHECK_FALSE(StyleParam::parseVec3(input, UnitSet{Unit::none, Unit::meter, Unit::pixel}, result));
    }

    SECTION("Invalid numbers") {
        auto input = YAML::Load("[0m, px, 100]");
        CHECK_FALSE(StyleParam::parseVec3(input, UnitSet{Unit::none, Unit::meter, Unit::pixel}, result));
    }

    SECTION("Invalid units") {
        auto input = YAML::Load("[0m, 40px, 66s]");
        CHECK_FALSE(StyleParam::parseVec3(input, UnitSet{Unit::none, Unit::meter, Unit::pixel}, result));
    }
}
