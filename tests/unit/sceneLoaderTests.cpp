#include "catch.hpp"

#include "mockPlatform.h"
#include "scene/pointLight.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "style/material.h"
#include "style/polylineStyle.h"
#include "style/polygonStyle.h"
#include "style/style.h"

#include "yaml-cpp/yaml.h"

using namespace Tangram;
using YAML::Node;

TEST_CASE("Style with the same name as a built-in style are ignored") {
    MockPlatform platform;
    Scene scene{platform, std::make_unique<SceneOptions>(Url())};
    SceneLoader::loadStyle(scene, "polygons", Node());
    REQUIRE(scene.styles().size() == 0);

}

TEST_CASE("Correctly instantiate a style from a YAML configuration") {
    MockPlatform platform;
    Scene scene{platform, std::make_unique<SceneOptions>(Url())};

    scene.styles().emplace_back(new PolygonStyle("polygons"));
    scene.styles().emplace_back(new PolylineStyle("lines"));

    YAML::Node node = YAML::Load(R"END(
        animated: true
        texcoords: true
        base: lines
        mix: tools
        material:
            diffuse: .9
            emission: 0.0
        )END");

    SceneLoader::loadStyle(scene, "roads", node);

    auto& styles = scene.styles();

    REQUIRE(styles.size() == 3);
    REQUIRE(styles[0]->getName() == "polygons");
    REQUIRE(styles[1]->getName() == "lines");
    REQUIRE(styles[2]->getName() == "roads");

    REQUIRE(styles[2]->isAnimated() == true);

    REQUIRE(styles[2]->getMaterial().hasEmission() == true);
    REQUIRE(styles[2]->getMaterial().hasDiffuse() == true);
    REQUIRE(styles[2]->getMaterial().hasAmbient() == false);
    REQUIRE(styles[2]->getMaterial().hasSpecular() == false);
}

TEST_CASE("Test light parameter parsing") {
    YAML::Node node = YAML::Load("position: [100px, 0, 20m]");

    auto light(std::make_unique<PointLight>("light"));
    SceneLoader::parseLightPosition(node["position"], *light);

    auto pos = light->getPosition();

    REQUIRE(pos.value.x == 100);
    REQUIRE(pos.value.y == 0);
    REQUIRE(pos.value.z == 20);
    REQUIRE(pos.units[0] == Unit::pixel);
    REQUIRE(pos.units[1] == Unit::meter);
    REQUIRE(pos.units[2] == Unit::meter);
}
