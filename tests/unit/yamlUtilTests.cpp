#include "catch.hpp"

#include "util/yamlUtil.h"

using namespace Tangram;

TEST_CASE("YamlUtil functions correctly handle scalar node", "[YamlUtil]") {

    int defaultInt = 1234567, initialInt = 7654321, resultInt = initialInt;
    float defaultFloat = 12345.67f, initialFloat = 76.54321f, resultFloat = initialFloat;
    double defaultDouble = 12345.67, initialDouble = 76.54321, resultDouble = initialDouble;
    bool defaultBool = true, initialBool = false, resultBool = initialBool;

    SECTION("Empty String Node") {
        YAML::Node node("");

        CHECK_FALSE(YamlUtil::getInt(node, resultInt));
        CHECK(resultInt == initialInt);
        CHECK(YamlUtil::getIntOrDefault(node, defaultInt) == defaultInt);

        CHECK_FALSE(YamlUtil::getFloat(node, resultFloat));
        CHECK(resultFloat == initialFloat);
        CHECK(YamlUtil::getFloatOrDefault(node, defaultFloat) == defaultFloat);

        CHECK_FALSE(YamlUtil::getDouble(node, resultDouble));
        CHECK(resultDouble == initialDouble);
        CHECK(YamlUtil::getDoubleOrDefault(node, defaultDouble) == defaultDouble);

        CHECK_FALSE(YamlUtil::getBool(node, resultBool));
        CHECK(resultBool == initialBool);
        CHECK(YamlUtil::getBoolOrDefault(node, defaultBool) == defaultBool);
    }

    SECTION("String Node") {
        YAML::Node node("I saw 3 ships go sailing by");

        CHECK_FALSE(YamlUtil::getInt(node, resultInt));
        CHECK(resultInt == initialInt);
        CHECK(YamlUtil::getIntOrDefault(node, defaultInt) == defaultInt);

        CHECK_FALSE(YamlUtil::getFloat(node, resultFloat));
        CHECK(resultFloat == initialFloat);
        CHECK(YamlUtil::getFloatOrDefault(node, defaultFloat) == defaultFloat);

        CHECK_FALSE(YamlUtil::getDouble(node, resultDouble));
        CHECK(resultDouble == initialDouble);
        CHECK(YamlUtil::getDoubleOrDefault(node, defaultDouble) == defaultDouble);

        CHECK_FALSE(YamlUtil::getBool(node, resultBool));
        CHECK(resultBool == initialBool);
        CHECK(YamlUtil::getBoolOrDefault(node, defaultBool) == defaultBool);
    }

    SECTION("Number Node") {
        YAML::Node node("3.14159");
        double valueDouble = 3.14159;
        float valueFloat = 3.14159f;
        int valueInt = 3;

        CHECK(YamlUtil::getInt(node, resultInt));
        CHECK(resultInt == valueInt);
        CHECK(YamlUtil::getIntOrDefault(node, defaultInt) == valueInt);

        CHECK(YamlUtil::getFloat(node, resultFloat));
        CHECK(resultFloat == valueFloat);
        CHECK(YamlUtil::getFloatOrDefault(node, defaultFloat) == valueFloat);

        CHECK(YamlUtil::getDouble(node, resultDouble));
        CHECK(resultDouble == valueDouble);
        CHECK(YamlUtil::getDoubleOrDefault(node, defaultDouble) == valueDouble);

        CHECK_FALSE(YamlUtil::getBool(node, resultBool));
        CHECK(resultBool == initialBool);
        CHECK(YamlUtil::getBoolOrDefault(node, defaultBool) == defaultBool);
    }

    SECTION("Integer Number Node") {
        YAML::Node node("8675309");
        double valueDouble = 8675309.0;
        float valueFloat = 8675309.f;
        int valueInt = 8675309;

        CHECK(YamlUtil::getInt(node, resultInt));
        CHECK(resultInt == valueInt);
        CHECK(YamlUtil::getIntOrDefault(node, defaultInt) == valueInt);

        CHECK(YamlUtil::getFloat(node, resultFloat));
        CHECK(resultFloat == valueFloat);
        CHECK(YamlUtil::getFloatOrDefault(node, defaultFloat) == valueFloat);

        CHECK(YamlUtil::getDouble(node, resultDouble));
        CHECK(resultDouble == valueDouble);
        CHECK(YamlUtil::getDoubleOrDefault(node, defaultDouble) == valueDouble);

        CHECK_FALSE(YamlUtil::getBool(node, resultBool));
        CHECK(resultBool == initialBool);
        CHECK(YamlUtil::getBoolOrDefault(node, defaultBool) == defaultBool);
    }

    SECTION("Bool Node") {
        YAML::Node node("true");
        bool valueBool = true;

        CHECK_FALSE(YamlUtil::getInt(node, resultInt));
        CHECK(resultInt == initialInt);
        CHECK(YamlUtil::getIntOrDefault(node, defaultInt) == defaultInt);

        CHECK_FALSE(YamlUtil::getFloat(node, resultFloat));
        CHECK(resultFloat == initialFloat);
        CHECK(YamlUtil::getFloatOrDefault(node, defaultFloat) == defaultFloat);

        CHECK_FALSE(YamlUtil::getDouble(node, resultDouble));
        CHECK(resultDouble == initialDouble);
        CHECK(YamlUtil::getDoubleOrDefault(node, defaultDouble) == defaultDouble);

        CHECK(YamlUtil::getBool(node, resultBool));
        CHECK(resultBool == valueBool);
        CHECK(YamlUtil::getBoolOrDefault(node, defaultBool) == valueBool);
    }

    SECTION("Number With Trailing Junk Node") {
        YAML::Node node("3.14159 radians");

        CHECK_FALSE(YamlUtil::getInt(node, resultInt));
        CHECK(resultInt == initialInt);
        CHECK(YamlUtil::getIntOrDefault(node, defaultInt) == defaultInt);

        CHECK_FALSE(YamlUtil::getFloat(node, resultFloat));
        CHECK(resultFloat == initialFloat);
        CHECK(YamlUtil::getFloatOrDefault(node, defaultFloat) == defaultFloat);

        CHECK_FALSE(YamlUtil::getDouble(node, resultDouble));
        CHECK(resultDouble == initialDouble);
        CHECK(YamlUtil::getDoubleOrDefault(node, defaultDouble) == defaultDouble);

        CHECK_FALSE(YamlUtil::getBool(node, resultBool));
        CHECK(resultBool == initialBool);
        CHECK(YamlUtil::getBoolOrDefault(node, defaultBool) == defaultBool);
    }

    SECTION("Number With Allowed Trailing Junk Node") {
        YAML::Node node("3.14159 radians");
        double valueDouble = 3.14159;
        float valueFloat = 3.14159f;
        int valueInt = 3;

        CHECK(YamlUtil::getInt(node, resultInt, true));
        CHECK(resultInt == valueInt);
        CHECK(YamlUtil::getIntOrDefault(node, defaultInt, true) == valueInt);

        CHECK(YamlUtil::getFloat(node, resultFloat, true));
        CHECK(resultFloat == valueFloat);
        CHECK(YamlUtil::getFloatOrDefault(node, defaultFloat, true) == valueFloat);

        CHECK(YamlUtil::getDouble(node, resultDouble, true));
        CHECK(resultDouble == valueDouble);
        CHECK(YamlUtil::getDoubleOrDefault(node, defaultDouble, true) == valueDouble);

        CHECK_FALSE(YamlUtil::getBool(node, resultBool));
        CHECK(resultBool == initialBool);
        CHECK(YamlUtil::getBoolOrDefault(node, defaultBool) == defaultBool);
    }

    SECTION("Non-scalar Node") {
        auto node = GENERATE(
                YAML::Node(YAML::NodeType::Null),
                YAML::Node(YAML::NodeType::Sequence),
                YAML::Node(YAML::NodeType::Map),
                YAML::Node(YAML::NodeType::Undefined)
                );

        CHECK_FALSE(YamlUtil::getInt(node, resultInt));
        CHECK(resultInt == initialInt);
        CHECK(YamlUtil::getIntOrDefault(node, defaultInt) == defaultInt);

        CHECK_FALSE(YamlUtil::getFloat(node, resultFloat));
        CHECK(resultFloat == initialFloat);
        CHECK(YamlUtil::getFloatOrDefault(node, defaultFloat) == defaultFloat);

        CHECK_FALSE(YamlUtil::getDouble(node, resultDouble));
        CHECK(resultDouble == initialDouble);
        CHECK(YamlUtil::getDoubleOrDefault(node, defaultDouble) == defaultDouble);

        CHECK_FALSE(YamlUtil::getBool(node, resultBool));
        CHECK(resultBool == initialBool);
        CHECK(YamlUtil::getBoolOrDefault(node, defaultBool) == defaultBool);
    }
}
