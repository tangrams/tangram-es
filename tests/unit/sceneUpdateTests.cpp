#include "catch.hpp"

#include "log.h"
#include "map.h"
#include "mockPlatform.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "style/style.h"

#include "yaml-cpp/yaml.h"

using YAML::Node;
using namespace Tangram;

const static std::string sceneString = R"END(
global:
    a: global_a_value
    b: global_b_value
map:
    a: map_a_value
    b: global.b
seq:
    - seq_0_value
    - global.a
nest:
    map:
        a: nest_map_a_value
        b: nest_map_b_value
    seq:
        - nest_seq_0_value
        - nest_seq_1_value
)END";

bool loadConfig(const std::string& _sceneString, Node& root) {

    try { root = YAML::Load(_sceneString); }
    catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
        return false;
    }
    return true;
}

TEST_CASE("Apply scene update to a top-level node") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"map", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(config, updates);
    const Node& root = config;
    CHECK(root["map"].Scalar() == "new_value");
}

TEST_CASE("Apply scene update to a map entry") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"map.a", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(config, updates);
    const Node& root = config;
    CHECK(root["map"]["a"].Scalar() == "new_value");
    // Check that nearby values are unchanged.
    CHECK(root["map"]["b"].Scalar() == "global.b");
}

TEST_CASE("Apply scene update to a nested map entry") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"nest.map.a", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(config, updates);
    const Node& root = config;
    CHECK(root["nest"]["map"]["a"].Scalar() == "new_value");
    // Check that nearby values are unchanged.
    CHECK(root["nest"]["map"]["b"].Scalar() == "nest_map_b_value");
}

TEST_CASE("Apply scene update to a sequence node") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"seq", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(config, updates);
    const Node& root = config;
    CHECK(root["seq"].Scalar() == "new_value");
}

TEST_CASE("Apply scene update to a nested sequence node") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"nest.seq", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(config, updates);
    const Node& root = config;
    CHECK(root["nest"]["seq"].Scalar() == "new_value");
    // Check that nearby values are unchanged.
    CHECK(root["nest"]["map"]["a"].Scalar() == "nest_map_a_value");
}

TEST_CASE("Apply scene update to a new map entry") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"map.c", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(config, updates);
    const Node& root = config;
    CHECK(root["map"]["c"].Scalar() == "new_value");
    // Check that nearby values are unchanged.
    CHECK(root["map"]["b"].Scalar() == "global.b");
}

TEST_CASE("Do not apply scene update to a non-existent node") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"none.a", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(config, updates);
    const Node& root = config;
    REQUIRE(!root["none"]);
}

TEST_CASE("Apply scene update that removes a node") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"nest.map", "null"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(config, updates);
    const Node& root = config;
    CHECK(!root["nest"]["map"]["a"]);
    CHECK(root["nest"]["map"].IsNull());
    CHECK(root["nest"]["seq"]);
}

TEST_CASE("Apply multiple scene updates in order of request") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"map.a", "first_value"}, {"map.a", "second_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(config, updates);
    const Node& root = config;
    CHECK(root["map"]["a"].Scalar() == "second_value");
    // Check that nearby values are unchanged.
    CHECK(root["map"]["b"].Scalar() == "global.b");
}

TEST_CASE("Apply and propogate repeated global value updates") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    Node& root = config;
    // Apply initial globals.
    SceneLoader::applyGlobals(config);
    CHECK(root["seq"][1].Scalar() == "global_a_value");
    CHECK(root["map"]["b"].Scalar() == "global_b_value");
    // Add an update.
    std::vector<SceneUpdate> updates = {{"global.b", "new_global_b_value"}};
    // Apply the update.
    SceneLoader::applyUpdates(config, updates);
    CHECK(root["global"]["b"].Scalar() == "new_global_b_value");

    // // Apply updated globals.
    // SceneLoader::applyGlobals(config);
    // CHECK(root["seq"][1].Scalar() == "global_a_value");
    // CHECK(root["map"]["b"].Scalar() == "new_global_b_value");
    // // Add an update.
    // updates = {{"global.b", "newer_global_b_value"}};
    // // Apply the update.
    // SceneLoader::applyUpdates(config, updates);
    // CHECK(root["global"]["b"].Scalar() == "newer_global_b_value");
    // // Apply updated globals.
    // SceneLoader::applyGlobals(config);
    // CHECK(root["seq"][1].Scalar() == "global_a_value");
    // CHECK(root["map"]["b"].Scalar() == "newer_global_b_value");
}

TEST_CASE("Regression: scene update requesting a sequence from a scalar") {
    Node config;
    REQUIRE(loadConfig(sceneString, config));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"map.a#0", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(config, updates);
    const Node& root = config;

    // causes yaml exception 'operator[] call on a scalar'
}

TEST_CASE("Scene update statuses") {
    {
        Node config;
        REQUIRE(loadConfig(sceneString, config));
        std::vector<SceneUpdate> updates = {{"map.a", "{ first_value"}};
        CHECK(SceneLoader::applyUpdates(config, updates).error == Error::scene_update_value_yaml_syntax_error);
    }
    {
        Node config;
        REQUIRE(loadConfig(sceneString, config));
        std::vector<SceneUpdate> updates = {{"someKey.somePath", "someValue"}};
        CHECK(SceneLoader::applyUpdates(config, updates).error == Error::scene_update_path_not_found);
    }
    {
        Node config;
        REQUIRE(loadConfig(sceneString, config));
        std::vector<SceneUpdate> updates = {{"map.a.map_a_value", "someValue"}};
        CHECK(SceneLoader::applyUpdates(config, updates).error == Error::scene_update_path_not_found);
    }
    {
        Node config;
        REQUIRE(loadConfig(sceneString, config));
        std::vector<SceneUpdate> updates = {{"!map#0", "first_value"}};
        CHECK(SceneLoader::applyUpdates(config, updates).error == Error::scene_update_path_not_found);
    }
    {
        Node config;
        REQUIRE(loadConfig(sceneString, config));
        std::vector<SceneUpdate>  updates = {{"key_not_existing", "first_value"}};
        CHECK(SceneLoader::applyUpdates(config, updates).error == Error::none);
    }
    {
        Node config;
        REQUIRE(loadConfig(sceneString, config));
        std::vector<SceneUpdate> updates = {{"!map#0", "{ first_value"}};
        CHECK(SceneLoader::applyUpdates(config, updates).error == Error::scene_update_value_yaml_syntax_error);
    }
}
