#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "yaml-cpp/yaml.h"
#include "sceneLoader.h"
#include "scene/scene.h"
#include "style/style.h"
#include "style/polylineStyle.h"
#include "style/polygonStyle.h"

#include "platform.h"

using namespace Tangram;
using YAML::Node;

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

    SceneLoader::loadStyle(*n, node, scene);

    auto& styles = scene.styles();

    REQUIRE(styles.size() == 3);
    REQUIRE(styles[0]->getName() == "polygons");
    REQUIRE(styles[1]->getName() == "lines");
    REQUIRE(styles[2]->getName() == "roads");

    REQUIRE(styles[2]->isAnimated() == true);

    REQUIRE(styles[2]->getMaterial()->hasEmission() == true);
    REQUIRE(styles[2]->getMaterial()->hasDiffuse() == true);
    REQUIRE(styles[2]->getMaterial()->hasAmbient() == true);
    REQUIRE(styles[2]->getMaterial()->hasSpecular() == false);
}

TEST_CASE( "Test style merging - shader global" ) {

    Scene scene;
    scene.styles().emplace_back(new PolygonStyle("polygons"));

    YAML::Node node = YAML::Load(R"END(
    styles:
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

    {
        auto styles = node["styles"];

        for (const auto& style : styles) {
            SceneLoader::loadStyle(style, styles, scene);
        }
    }

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
    styles:
      mixedstyle:
        base: polygons
        mix: [ kitchensink, toolbox ]
        shaders:
          blocks:
            global: mixed
      toolbox:
        mix: kitchensink
        shaders:
          blocks:
            global: toolbox
      kitchensink:
        shaders:
          blocks:
            global: kitchensink
     )END");

    {
        auto styles = node["styles"];

        for (const auto& style : styles) {
            SceneLoader::loadStyle(style, styles, scene);
        }
    }

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
    REQUIRE(styles[1]->getShaderProgram()->getSourceBlocks()["global"][0] == "\nkitchensink\ntoolbox\nmixed");
}
