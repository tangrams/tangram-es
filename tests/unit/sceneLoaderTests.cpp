#include "catch.hpp"

#include "yaml-cpp/yaml.h"
#include "gl/shaderProgram.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"
#include "style/material.h"
#include "style/style.h"
#include "style/polylineStyle.h"
#include "style/polygonStyle.h"

#include "platform.h"

using namespace Tangram;
using YAML::Node;

TEST_CASE( "Test style loading with builin style name" ) {
    Scene scene;

    YAML::Node node = YAML::Load(R"END(
    polygons:
      base: polyons
     )END");

    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 1);

    const auto& n = node.begin();
    std::unordered_set<std::string> mixedStyles;
    SceneLoader::loadStyle("polygons", node, scene, mixedStyles);
    // builtin should not be added
    REQUIRE(scene.styles().size() == 0);
}

TEST_CASE( "Test style loading" ) {
    Scene scene;

    scene.styles().emplace_back(new PolygonStyle("polygons"));
    scene.styles().emplace_back(new PolylineStyle("lines"));

    YAML::Node node = YAML::Load(R"END(
    roads:
      animated: true
      texcoords: true
      base: lines
      mix: tools
      material:
        diffuse: .9
        emission: 0.0
     )END");

    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 1);

    // logMsg("Node:\n'%s'\n", Dump(node).c_str());

    const auto& n = node.begin();

    std::unordered_set<std::string> mixedStyles;
    SceneLoader::loadStyle("roads", node, scene, mixedStyles);

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

TEST_CASE( "Test style merging - shader global" ) {

    Scene scene;
    scene.styles().emplace_back(new PolygonStyle("polygons"));

    YAML::Node node = YAML::Load(R"END(
      mixedstyle:
        base: polygons
        mix: tools
        shaders:
          blocks:
            global: mixed
      tools:
        shaders:
          blocks:
            global: tools
     )END");

    std::unordered_set<std::string> mixedStyles;

    SceneLoader::loadStyle("mixedstyle", node, scene, mixedStyles);
    SceneLoader::loadStyle("tools", node, scene, mixedStyles);

    auto& styles = scene.styles();

    REQUIRE(styles.size() == 2);

    for (auto& style : styles) {
    for (auto& block : style->getShaderProgram()->getSourceBlocks()) {
        logMsg("block '%s'\n", block.first.c_str());
        for (auto& val : block.second)
            logMsg("- '%s'\n", val.c_str());

    }
    }

    REQUIRE(styles[1]->getShaderProgram()->getSourceBlocks()["global"].empty() == false);
}

TEST_CASE( "Test style merging two levels - shader global" ) {

    Scene scene;
    scene.styles().emplace_back(new PolygonStyle("polygons"));

    YAML::Node node = YAML::Load(R"END(
  blueprint:
    base: polygons
    shaders:
       blocks:
        global: |
          blueprint...

  base:
    base: polygons
    mix: blueprint
    lighting: false

  buildings:
    base: polygons
    mix: blueprint
    texcoords: true
    lighting: false
    shaders:
      blocks:
        color: |
          color...

     )END");

    std::unordered_set<std::string> mixedStyles;

    SceneLoader::loadStyle("blueprint", node, scene, mixedStyles);
    SceneLoader::loadStyle("base", node, scene, mixedStyles);
    SceneLoader::loadStyle("buildings", node, scene, mixedStyles);

    auto& styles = scene.styles();
    REQUIRE(styles.size() == 4);

    REQUIRE(styles[3]->getShaderProgram()->getSourceBlocks()["global"].empty() == false);
    REQUIRE(styles[3]->getShaderProgram()->getSourceBlocks()["global"][0] == "blueprint...\n");
    REQUIRE(styles[3]->getShaderProgram()->getSourceBlocks()["color"][0] == "color...\n");
}
