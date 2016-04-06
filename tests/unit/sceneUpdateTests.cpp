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

TEST_CASE("Scene update for cameras scene block") {
    Scene scene("scene.yaml");

    auto sceneString = stringFromFile(setResourceRoot(path).c_str(), PathType::resource);

    REQUIRE(!sceneString.empty());

    Node root;
    REQUIRE(SceneLoader::loadScene(sceneString, scene, root));
    REQUIRE(root["cameras"]["iso-camera"]["active"].Scalar() == "false");

    // Update
    scene.setComponent("cameras.iso-camera.active", "true");
    scene.setComponent("cameras.iso-camera.type", "perspective");

    // Tangram apply scene updates, reload the scene
    REQUIRE(SceneLoader::loadScene(sceneString, scene, root));
    scene.clearUserDefines();

    REQUIRE(root["cameras"]["iso-camera"]["active"].Scalar() == "true");
    REQUIRE(root["cameras"]["iso-camera"]["type"].Scalar() == "perspective");
}

TEST_CASE("Scene update for light scene block") {
    Scene scene("scene.yaml");

    auto sceneString = stringFromFile(setResourceRoot(path).c_str(), PathType::resource);

    REQUIRE(!sceneString.empty());

    Node root;
    REQUIRE(SceneLoader::loadScene(sceneString, scene, root));

    // Update
    scene.setComponent("lights.light1.ambient", "0.9");
    scene.setComponent("lights.light1.type", "spotlight");
    scene.setComponent("lights.light1.origin", "ground");

    // Tangram apply scene updates, reload the scene
    REQUIRE(SceneLoader::loadScene(sceneString, scene, root));
    scene.clearUserDefines();

    REQUIRE(root["lights"]["light1"]["ambient"].Scalar() == "0.9");
    REQUIRE(root["lights"]["light1"]["type"].Scalar() == "spotlight");
    REQUIRE(root["lights"]["light1"]["origin"].Scalar() == "ground");
}

