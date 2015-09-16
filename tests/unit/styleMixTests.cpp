#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <iostream>
#include <vector>
#include <string>

#include "data/filters.h"
#include "yaml-cpp/yaml.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"

using namespace Tangram;
using YAML::Node;

// shader property overwrite test
// shader extensions test
// shader block test

TEST_CASE( "Style Mixing Test1: propMerge Tests (recursive overWrite properties)", "[mixing][core][yaml]") {
    SceneLoader sceneLoader;
    Node node = YAML::Load("Nodes: { \
                                Node1: { \
                                    prop1: { \
                                        subProp1: { \
                                            tag1: value1 } \
                                    }, \
                                    prop2: { \
                                        subProp3: { \
                                            tag2: value2 } \
                                    } \
                                }, \
                                Node2: { \
                                    prop1: { \
                                        subProp1: { \
                                            tag3: value3 } \
                                    }, \
                                    prop2: { \
                                        subProp2: value_scalar, \
                                        subProp3: value_scalar2 \
                                    } \
                                }, \
                                Node3: { \
                                    prop1: { \
                                        subProp1: {\
                                            tag1: value1_3 \
                                        } \
                                    }, \
                                    prop2: { \
                                        subProp2: [v1, v2], \
                                        subProp3: { \
                                            tag4: value4 \
                                        } \
                                   } \
                                }, \
                            }")["Nodes"];

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
        mixedNode[property] = sceneLoader.propMerge(property, { node["Node1"], node["Node2"], node["Node3"] });
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

TEST_CASE( "Style Mixing Test1: propMerge Tests (overWrite properties)", "[mixing][core][yaml]") {
    SceneLoader sceneLoader;
    Node node = YAML::Load("Nodes: { \
                                Node1: { \
                                    prop1: value1 }, \
                                Node2: { \
                                    prop1: value1_2, \
                                    prop2: value2 }, \
                                Node3: { \
                                    prop1: value1_3 } \
                                }")["Nodes"];

    // NodeMix:
    //      prop1: value1_3
    //      prop2: value2

    Node mixedNode;
    for (auto& property : {"prop1", "prop2"}) {
        mixedNode[property] = sceneLoader.propMerge(property, { node["Node1"], node["Node2"], node["Node3"] });
    }

    REQUIRE(mixedNode["prop1"].as<std::string>() == "value1_3");
    REQUIRE(mixedNode["prop2"].as<std::string>() == "value2");
}

TEST_CASE( "Style Mixing Test1: propMerge Tests (boolean properties)", "[mixing][core][yaml]") {

    SceneLoader sceneLoader;
    Node node = YAML::Load("Nodes: { \
                                Node1: { \
                                    prop1: false }, \
                                Node2: { \
                                    prop1: true }, \
                                Node3: { \
                                    prop1: false } \
                            }")["Nodes"];

    // NodeMix:
    //      prop1: true

    Node mixedNode;
    for (auto& property : {"prop1"}) {
        mixedNode[property] = sceneLoader.propMerge(property, { node["Node1"], node["Node2"], node["Node3"] });
    }

    REQUIRE(mixedNode["prop1"].as<bool>());
}


