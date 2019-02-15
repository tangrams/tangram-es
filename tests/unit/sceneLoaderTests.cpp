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
    REQUIRE(SceneLoader::loadStyle("polygons", Node()) == nullptr);
}

TEST_CASE("Correctly instantiate a style from a YAML configuration") {

    Scene::Styles styles;
    styles.push_back(std::make_unique<PolygonStyle>("polygons"));
    styles.push_back(std::make_unique<PolylineStyle>("lines"));

    YAML::Node node = YAML::Load(R"END(
        animated: true
        texcoords: true
        base: lines
        mix: tools
        material:
            diffuse: .9
            emission: 0.0
        )END");

    SceneTextures textures;
    styles.push_back(SceneLoader::loadStyle("roads", node));
    SceneLoader::loadStyleProps(node, *styles.back().get(), textures);

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
