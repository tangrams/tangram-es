#include "catch.hpp"

#include "yaml-cpp/yaml.h"
#include "scene/sceneLoader.h"
#include "style/style.h"
#include "scene/scene.h"
#include "platform.h"
#include "tangram.h"

using YAML::Node;
using namespace Tangram;

const char* path = "scene.yaml";

TEST_CASE("Scene update tests") {
    Scene scene("scene.yaml");

    auto sceneString = stringFromFile(setResourceRoot(path).c_str(), PathType::resource);

    REQUIRE(!sceneString.empty());

    Node root;
    REQUIRE(SceneLoader::loadScene(sceneString, scene, root));

    // Update
    scene.queueComponentUpdate("lights.light1.ambient", "0.9");
    scene.queueComponentUpdate("lights.light1.type", "spotlight");
    scene.queueComponentUpdate("lights.light1.origin", "ground");
    scene.queueComponentUpdate("layers.poi_icons.draw.icons.interactive", "false");
    scene.queueComponentUpdate("styles.heightglow.shaders.uniforms.u_time_expand", "5.0");
    scene.queueComponentUpdate("cameras.iso-camera.active", "true");
    scene.queueComponentUpdate("cameras.iso-camera.type", "perspective");
    scene.queueComponentUpdate("global.default_order", "function() { return 0.0; }");
    scene.queueComponentUpdate("global.non_existing_property0", "true");
    scene.queueComponentUpdate("global.non_existing_property1.non_existing_property_deep", "true");

    // Tangram apply scene updates, reload the scene
    REQUIRE(SceneLoader::loadScene(sceneString, scene, root));
    scene.clearUserDefines();

    REQUIRE(root["lights"]["light1"]["ambient"].Scalar() == "0.9");
    REQUIRE(root["lights"]["light1"]["type"].Scalar() == "spotlight");
    REQUIRE(root["lights"]["light1"]["origin"].Scalar() == "ground");
    REQUIRE(root["layers"]["poi_icons"]["draw"]["icons"]["interactive"].Scalar() == "false");
    REQUIRE(root["styles"]["heightglow"]["shaders"]["uniforms"]["u_time_expand"].Scalar() == "5.0");
    REQUIRE(root["styles"]["heightglowline"]["shaders"]["uniforms"]["u_time_expand"].Scalar() == "5.0");
    REQUIRE(root["cameras"]["iso-camera"]["active"].Scalar() == "true");
    REQUIRE(root["cameras"]["iso-camera"]["type"].Scalar() == "perspective");
    REQUIRE(root["global"]["default_order"].Scalar() == "function() { return 0.0; }");
    REQUIRE(root["global"]["non_existing_property0"].Scalar() == "true");
    REQUIRE(!root["global"]["non_existing_property1"]);
}

