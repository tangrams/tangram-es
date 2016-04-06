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

TEST_CASE("") {
    Scene scene("scene.yaml");

    auto sceneString = stringFromFile(setResourceRoot(path).c_str(), PathType::resource);

    REQUIRE(!sceneString.empty());

    Node root;
    REQUIRE(SceneLoader::loadScene(sceneString, scene, root));
    REQUIRE(root["cameras"]["iso-camera"]["active"].Scalar() == "false");

    // Update
    scene.setComponent("cameras.iso-camera.active", "true");

    // Tangram apply scene updates, reload the scene
    REQUIRE(SceneLoader::loadScene(sceneString, scene, root));
    scene.clearUserDefines();

    REQUIRE(root["cameras"]["iso-camera"]["active"].Scalar() == "true");
}

