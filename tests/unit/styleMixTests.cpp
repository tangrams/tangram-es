#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <vector>
#include <string>

#include "yaml-cpp/yaml.h"
#include "scene/filters.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"

using namespace Tangram;
using YAML::Node;

TEST_CASE( "Style Mixing Test: Actual Property recursive merge check!!!", "[mixing][core][yaml]") {
    std::unordered_set<std::string> mixedStyles;
    std::vector<Node> mix;
    Node mixNode;
    Scene scene;
    Node node = YAML::Load(R"END(
        styleA:
            material: A
        styleB:
            mix: styleA
            base: wheresmymix?
        )END");

    SceneLoader::loadStyle("styleA", node, scene, mixedStyles);
    mixNode = node["styleA"];

    SceneLoader::loadStyle("styleB", node, scene, mixedStyles);
    mixNode = node["styleB"];

    REQUIRE(mixNode["material"].IsScalar());
    REQUIRE(mixNode["material"].as<std::string>() ==  "A");
}


TEST_CASE( "Style Mixing Test: Concrete Overwrite value check", "[mixing][core][yaml]") {
    std::unordered_set<std::string> mixedStyles;
    std::vector<Node> mix;
    Node mixNode;
    Scene scene;

    Node node = YAML::Load(R"END(
        StyleA:
            mix: StyleB
        StyleB:
            mix: StyleC
        StyleC:
            material: valueC
        )END");

    SceneLoader::loadStyle("StyleA", node, scene, mixedStyles);
    mixNode = node["StyleA"];
    REQUIRE(mixNode["material"].as<std::string>() == "valueC");
}

TEST_CASE( "Style Mixing Test: Nested Style Mixin Nodes", "[mixing][core][yaml]") {
    std::unordered_set<std::string> uniqueStyles;
    std::unordered_set<std::string> mixedStyles;
    std::vector<Node> mix;
    Node mixNode;
    Scene scene;

    Node styles = YAML::Load(R"END(
        styleA:
        styleB:
        styleC:
            mix: [styleA, styleB]
        styleD:
            mix: [styleC, styleA]
        styleE:
            mix: [styleA, styleB, styleF]
        styleF:
            mix: styleA
        )END");

    // From SceneLoader::loadStyle()
    auto countMixes = [&](const std::string& styleName) {

        mixedStyles.insert(styleName);
        Node styleNode = styles[styleName];

        std::vector<Node> mixes = SceneLoader::getMixins(styleNode, styles, scene, mixedStyles);

        // Finally through our self into the mix!
        mixes.push_back(styleNode);

        Node mixedStyleNode = SceneLoader::mixStyles(mixes);

        // Remember that this style has been processed and
        // update styleNode with mixedStyleNode (for future uses)
        styles[styleName] = mixedStyleNode;

        return mixes.size();
    };

    int count = countMixes("styleA");
    REQUIRE(count == 1);

    count = countMixes("styleB");
    REQUIRE(count == 1);

    count = countMixes("styleB");
    REQUIRE(count == 1);

    count = countMixes("styleC");
    REQUIRE(count == 3);

    count = countMixes("styleD");
    REQUIRE(count == 3);

    count = countMixes("styleE");
    REQUIRE(count == 4);

    count = countMixes("styleF");
    REQUIRE(count == 1);

}


TEST_CASE( "Style Mixing Test: Shader Extensions Merging", "[mixing][core][yaml]") {
    Node node = YAML::Load(R"END(
        Node1:
            shaders:
                extensions: extension1
        Node2:
            shaders:
                extensions: [extension1, extension2, extension3]
        Node3:
            shaders:
                extensions: extension3
        Node4:
            shaders:
                extensions: [extension4]
        )END");

    Node extNode;

    extNode = SceneLoader::shaderExtMerge( { node["Node1"] } );
    // extNode: [extension1]
    REQUIRE(extNode.size() == 1);
    REQUIRE(extNode[0].as<std::string>() == "extension1");

    extNode = SceneLoader::shaderExtMerge( { node["Node1"], node["Node2"], node["Node3"], node["Node4"] } );
    // extNode: [extension1, extension2, extension3, extension4]
    REQUIRE(extNode.size() == 4);
    REQUIRE(extNode[0].as<std::string>() == "extension1");
    REQUIRE(extNode[1].as<std::string>() == "extension2");
    REQUIRE(extNode[2].as<std::string>() == "extension3");
    REQUIRE(extNode[3].as<std::string>() == "extension4");

}

TEST_CASE( "Style Mixing Test: Shader Blocks Merging", "[mixing][core][yaml]") {

    Node node = YAML::Load(R"END(
        Node1:
            shaders:
                blocks:
                    color: colorBlockA;
                    normal: normalBlockA;
        Node2:
            shaders:
                blocks:
                    color: colorBlockB;
                    position: posBlockB;
                    global: globalBlockB;
        Node3:
            shaders:
                blocks:
                    global: globalBlockC;
                    filter: filterBlockC;
        )END");

    Node shaderBlocksNode = SceneLoader::shaderBlockMerge( { node["Node1"] } );
    // shaderBlocksNode:
    //          color: colorBlockA;
    //          normal: normalBlockA;

    REQUIRE(shaderBlocksNode["color"][0].as<std::string>() == "colorBlockA;");
    REQUIRE(shaderBlocksNode["normal"][0].as<std::string>() == "normalBlockA;");
    REQUIRE(!shaderBlocksNode["global"]);

    shaderBlocksNode = SceneLoader::shaderBlockMerge( { node["Node1"], node["Node2"], node["Node3"] } );

    // shaderBlocksNode:
    //          color: colorBlockA;\ncolorBlockB;
    //          normal: normalBlockA;
    //          position: posBlockB;
    //          global: globalBlockB;\nglobalBlockC;
    //          filter: filterBlockC;

    REQUIRE(shaderBlocksNode["color"][0].as<std::string>() == "colorBlockA;");
    REQUIRE(shaderBlocksNode["color"][1].as<std::string>() == "colorBlockB;");
    REQUIRE(shaderBlocksNode["normal"][0].as<std::string>() == "normalBlockA;");
    REQUIRE(shaderBlocksNode["position"][0].as<std::string>() == "posBlockB;");
    REQUIRE(shaderBlocksNode["global"][0].as<std::string>() == "globalBlockB;");
    REQUIRE(shaderBlocksNode["global"][1].as<std::string>() == "globalBlockC;");
    REQUIRE(shaderBlocksNode["filter"][0].as<std::string>() == "filterBlockC;");
}

TEST_CASE( "Style Mixing Test: propMerge Tests (recursive overWrite properties)", "[mixing][core][yaml]") {

    Node node = YAML::Load(R"END(
        Node1:
            prop1:
                subProp1:
                    tag1: value1
            prop2:
                subProp3:
                    tag2: value2
        Node2:
            prop1:
                subProp1:
                    tag3: value3
            prop2:
                subProp2: value_scalar
                subProp3: value_scalar2
        Node3:
            prop1:
                subProp1:
                    tag1: value1_3
            prop2:
                subProp2: [v1, v2]
                subProp3:
                    tag4: value4
        )END");

    // NodeMix:
    //      prop1:
    //          subProp1:
    //              tag1: value1_3
    //              tag3: value3
    //      prop2:
    //          subProp2: [v1, v2]
    //          subProp3:
    //              tag4: value4
    //              tag2: value2

    Node mixedNode;

    for (auto& property : {"prop1", "prop2"}) {
        mixedNode[property] = SceneLoader::propMerge(property, { node["Node1"], node["Node2"], node["Node3"] });
    }

    REQUIRE(mixedNode["prop1"].IsMap());
    REQUIRE(mixedNode["prop1"]["subProp1"].IsMap());
    REQUIRE(mixedNode["prop1"]["subProp1"]["tag1"].as<std::string>() ==  "value1_3");
    REQUIRE(mixedNode["prop1"]["subProp1"]["tag3"].as<std::string>() ==  "value3");
    REQUIRE(mixedNode["prop2"].IsMap());
    REQUIRE(mixedNode["prop2"]["subProp2"].IsSequence());
    REQUIRE(mixedNode["prop2"]["subProp2"].size() == 2);
    REQUIRE(mixedNode["prop2"]["subProp3"].IsMap());
    REQUIRE(mixedNode["prop2"]["subProp3"].size() == 2);
    REQUIRE(mixedNode["prop2"]["subProp3"]["tag4"].as<std::string>() == "value4");
    REQUIRE(mixedNode["prop2"]["subProp3"]["tag2"].as<std::string>() == "value2");
}

TEST_CASE( "Style Mixing Test: propMerge Tests (overWrite properties)", "[mixing][core][yaml]") {

    Node node = YAML::Load(R"END(
        Node1:
            prop1: value1
        Node2:
            prop1: value1_2
            prop2: value2
        Node3:
            prop1: value1_3
        )END");

    // NodeMix:
    //      prop1: value1_3
    //      prop2: value2

    Node mixedNode;
    for (auto& property : {"prop1", "prop2"}) {
        mixedNode[property] = SceneLoader::propMerge(property, { node["Node1"], node["Node2"], node["Node3"] });
    }

    REQUIRE(mixedNode["prop1"].as<std::string>() == "value1_3");
    REQUIRE(mixedNode["prop2"].as<std::string>() == "value2");

    // Verify that the original node was not modified
    REQUIRE(node["Node1"]["prop1"].as<std::string>() == "value1");
    REQUIRE(node["Node2"]["prop1"].as<std::string>() == "value1_2");
    REQUIRE(node["Node2"]["prop2"].as<std::string>() == "value2");
    REQUIRE(node["Node3"]["prop1"].as<std::string>() == "value1_3");

}

TEST_CASE( "Style Mixing Test: propOr Tests (boolean properties)", "[mixing][core][yaml]") {

    Node node = YAML::Load(R"END(
        Node1:
            prop1: false
        Node2:
            prop1: true
        Node3:
            prop1: false
        )END");

    // NodeMix:
    //      prop1: true

    Node mixedNode;
    for (auto& property : {"prop1"}) {
        mixedNode[property] = SceneLoader::propOr(property, { node["Node1"], node["Node2"], node["Node3"] });
    }

    REQUIRE(mixedNode["prop1"].as<bool>());

}

TEST_CASE( "Style Mixing Test: Actual Property recursive merge check - part 2", "[mixing][core][yaml]") {
    std::unordered_set<std::string> uniqueStyles;
    std::unordered_set<std::string> mixedStyles;
    Scene scene;

    std::vector<Node> mix;
    Node mixNode;
    Node node = YAML::Load(R"END(
        styleLighter:
            lighting: lighter
        stylePocket:
            mix: styleLighter
        stylePants:
            material: A
            mix: stylePocket
            # base: where's my lighter?
            base: polygons
        styleDude:
            mix: stylePants
            # base: got the lighter!
            base: polygons
        )END");

    SceneLoader::loadStyle("styleDude", node, scene, mixedStyles);
    mixNode = node["styleDude"];
    uniqueStyles.clear();

    REQUIRE(mixNode["material"].IsScalar());
    REQUIRE(mixNode["material"].as<std::string>() ==  "A");
    // REQUIRE(mixNode["base"].as<std::string>() ==  "got the lighter!");
    REQUIRE(mixNode["base"].as<std::string>() ==  "polygons");
    REQUIRE(mixNode["lighting"].as<std::string>() ==  "lighter");

    SceneLoader::loadStyle("stylePants", node, scene, mixedStyles);
    mixNode = node["stylePants"];
    uniqueStyles.clear();

    REQUIRE(mixNode["material"].IsScalar());
    REQUIRE(mixNode["material"].as<std::string>() ==  "A");
    // REQUIRE(mixNode["base"].as<std::string>() ==  "where's my lighter?");
    REQUIRE(mixNode["base"].as<std::string>() ==  "polygons");
    REQUIRE(mixNode["lighting"].as<std::string>() ==  "lighter");

}

TEST_CASE( "Style Mixing Test: Dont allow loops in mix inheritance", "[mixing][core][yaml]") {
    std::unordered_set<std::string> mixedStyles;
    Scene scene;

    std::vector<Node> mix;
    Node mixNode;
    Node node = YAML::Load(R"END(
        styleA:
            mix: styleB
            # base: A
            base: polygons
        styleB:
            mix: styleC
            lighting: B
        styleC:
            mix: styleA
            material: C
        )END");

    SceneLoader::loadStyle("styleA", node, scene, mixedStyles);
    mixNode = node["styleA"];

    REQUIRE(mixNode["base"].IsScalar());
    REQUIRE(mixNode["lighting"].IsScalar());
    REQUIRE(mixNode["material"].IsScalar());
    // REQUIRE(mixNode["base"].as<std::string>() ==  "A");
    REQUIRE(mixNode["base"].as<std::string>() ==  "polygons");
    REQUIRE(mixNode["lighting"].as<std::string>() ==  "B");
    REQUIRE(mixNode["material"].as<std::string>() ==  "C");

}
