#include "catch.hpp"

#include "yaml-cpp/yaml.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"
#include "style/material.h"
#include "style/style.h"
#include "style/polylineStyle.h"
#include "style/polygonStyle.h"

#include "platform.h"

using namespace Tangram;
using YAML::Node;

TEST_CASE("Style with the same name as a built-in style are ignored") {

    Scene scene("");
    SceneLoader::loadStyle("polygons", Node(), scene);
    REQUIRE(scene.styles().size() == 0);

}

TEST_CASE("Correctly instantiate a style from a YAML configuration") {
    Scene scene("");

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

    SceneLoader::loadStyle("roads", node, scene);

    auto& styles = scene.styles();

    REQUIRE(styles.size() == 3);
    REQUIRE(styles[0]->getName() == "polygons");
    REQUIRE(styles[1]->getName() == "lines");
    REQUIRE(styles[2]->getName() == "roads");

    REQUIRE(styles[2]->isAnimated() == true);

    REQUIRE(styles[2]->getMaterial()->hasEmission() == true);
    REQUIRE(styles[2]->getMaterial()->hasDiffuse() == true);
    REQUIRE(styles[2]->getMaterial()->hasAmbient() == false);
    REQUIRE(styles[2]->getMaterial()->hasSpecular() == false);
}
