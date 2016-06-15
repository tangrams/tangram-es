#include "catch.hpp"

#include "yaml-cpp/yaml.h"
#include "scene/sceneLoader.h"
#include "style/style.h"
#include "scene/scene.h"
#include "platform.h"
#include "tangram.h"

using YAML::Node;
using namespace Tangram;

const static std::string sceneString = R"END(
global:
    default_order: function() { return feature.sort_key; }

cameras:
    iso-camera:
        type: isometric
        active: false
    perspective-camera:
        type: perspective
        active: true

lights:
    light1:
        type: directional
        direction: [.1, .5, -1]
        diffuse: .7
        ambient: .5

styles:
    heightglow:
        base: polygons
        shaders:
            uniforms:
                u_time_expand: 10.0
    heightglowline:
        base: lines
        mix: heightglow

layers:
    poi_icons:
        draw:
            icons:
                interactive: true

)END";

TEST_CASE("Scene update tests") {
    Scene scene;

    REQUIRE(!sceneString.empty());

    REQUIRE(SceneLoader::loadConfig(sceneString, scene.config()));

    std::vector<Scene::Update> updates;
    // Update
    updates.push_back({"lights.light1.ambient", "0.9"});
    updates.push_back({"lights.light1.type", "spotlight"});
    updates.push_back({"lights.light1.origin", "ground"});
    updates.push_back({"layers.poi_icons.draw.icons.interactive", "false"});
    updates.push_back({"styles.heightglow.shaders.uniforms.u_time_expand", "5.0"});
    updates.push_back({"cameras.iso-camera.active", "true"});
    updates.push_back({"cameras.iso-camera.type", "perspective"});
    updates.push_back({"global.default_order", "function() { return 0.0; }"});
    updates.push_back({"global.non_existing_property0", "true"});
    updates.push_back({"global.non_existing_property1.non_existing_property_deep", "true"});

    // Tangram apply scene updates, reload the scene
    SceneLoader::applyUpdates(scene.config(), updates);

    const Node& root = scene.config();

    REQUIRE(root["lights"]["light1"]["ambient"].Scalar() == "0.9");
    REQUIRE(root["lights"]["light1"]["type"].Scalar() == "spotlight");
    REQUIRE(root["lights"]["light1"]["origin"].Scalar() == "ground");
    REQUIRE(root["layers"]["poi_icons"]["draw"]["icons"]["interactive"].Scalar() == "false");
    REQUIRE(root["styles"]["heightglow"]["shaders"]["uniforms"]["u_time_expand"].Scalar() == "5.0");
    REQUIRE(root["cameras"]["iso-camera"]["active"].Scalar() == "true");
    REQUIRE(root["cameras"]["iso-camera"]["type"].Scalar() == "perspective");
    REQUIRE(root["global"]["default_order"].Scalar() == "function() { return 0.0; }");
    REQUIRE(root["global"]["non_existing_property0"].Scalar() == "true");
    REQUIRE(!root["global"]["non_existing_property1"]);
}

TEST_CASE("Scene update tests, ensure update ordering is preserved") {
    Scene scene;
    std::vector<Scene::Update> updates;

    REQUIRE(!sceneString.empty());

    REQUIRE(SceneLoader::loadConfig(sceneString, scene.config()));

    // Update
    updates.push_back({"lights.light1.ambient", "0.9"});
    updates.push_back({"lights.light2.ambient", "0.0"});

    // Delete lights
    updates.push_back({"lights", "null"});
    updates.push_back({"lights.light2.ambient", "0.0"});

    // Tangram apply scene updates, reload the scene
    SceneLoader::applyUpdates(scene.config(), updates);

    const Node& root = scene.config();

    REQUIRE(!root["lights"]["light1"]);
    REQUIRE(!root["lights"]["light2"]);
}
