#include "catch.hpp"

#include <iostream>
#include <vector>
#include <string>

#include "yaml-cpp/yaml.h"
#include "scene/filters.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"
#include "util/variant.h"

using namespace Tangram;
using YAML::Node;

Scene scene;

TEST_CASE( "Style Uniforms Parsing and Injection Test: Float uniform value", "[StyleUniforms][core][yaml]") {

    Node node = YAML::Load(R"END(
        u_float: 0.5
        )END");

    auto uniformValues = SceneLoader::parseStyleUniforms(node["u_float"], scene);
    REQUIRE(uniformValues.value.is<float>());
    REQUIRE(uniformValues.value.get<float>() == 0.5);
    REQUIRE(uniformValues.type == "float");
}

TEST_CASE( "Style Uniforms Parsing and Injection Test: Boolean uniform value", "[StyleUniforms][core][yaml]") {

    Node node = YAML::Load(R"END(
        u_true: true
        u_false: false
        )END");

    auto uniformValues = SceneLoader::parseStyleUniforms(node["u_true"], scene);
    REQUIRE(uniformValues.value.is<bool>());
    REQUIRE(uniformValues.value.get<bool>() == 1);
    REQUIRE(uniformValues.type == "bool");

    uniformValues = SceneLoader::parseStyleUniforms(node["u_false"], scene);
    REQUIRE(uniformValues.value.is<bool>());
    REQUIRE(uniformValues.value.get<bool>() == 0);
    REQUIRE(uniformValues.type == "bool");
}

TEST_CASE( "Style Uniforms Parsing and Injection Test: vec2, vec3, vec4 uniform value", "[StyleUniforms][core][yaml]") {

    Node node = YAML::Load(R"END(
        u_vec2: [0.1, 0.2]
        u_vec3: [0.1, 0.2, 0.3]
        u_vec4: [0.1, 0.2, 0.3, 0.4]
        u_array: [0.1, 0.2, 0.3, 0.4, 0.5]
        )END");


    auto uniformValues = SceneLoader::parseStyleUniforms(node["u_vec2"], scene);
    REQUIRE(uniformValues.value.is<glm::vec2>());
    REQUIRE(uniformValues.value.get<glm::vec2>().x == 0.1f);
    REQUIRE(uniformValues.value.get<glm::vec2>().y == 0.2f);
    REQUIRE(uniformValues.type == "vec2");

    uniformValues = SceneLoader::parseStyleUniforms(node["u_vec3"], scene);
    REQUIRE(uniformValues.value.is<glm::vec3>());
    REQUIRE(uniformValues.value.get<glm::vec3>().x == 0.1f);
    REQUIRE(uniformValues.value.get<glm::vec3>().y == 0.2f);
    REQUIRE(uniformValues.value.get<glm::vec3>().z == 0.3f);
    REQUIRE(uniformValues.type == "vec3");

    uniformValues = SceneLoader::parseStyleUniforms(node["u_vec4"], scene);
    REQUIRE(uniformValues.value.is<glm::vec4>());
    REQUIRE(uniformValues.value.get<glm::vec4>().x == 0.1f);
    REQUIRE(uniformValues.value.get<glm::vec4>().y == 0.2f);
    REQUIRE(uniformValues.value.get<glm::vec4>().z == 0.3f);
    REQUIRE(uniformValues.value.get<glm::vec4>().w == 0.4f);
    REQUIRE(uniformValues.type == "vec4");

    uniformValues = SceneLoader::parseStyleUniforms(node["u_array"], scene);
    REQUIRE(uniformValues.value.is<UniformArray>());
    REQUIRE(uniformValues.value.get<UniformArray>()[0] == 0.1f);
    REQUIRE(uniformValues.value.get<UniformArray>()[1] == 0.2f);
    REQUIRE(uniformValues.value.get<UniformArray>()[2] == 0.3f);
    REQUIRE(uniformValues.value.get<UniformArray>()[3] == 0.4f);
    REQUIRE(uniformValues.value.get<UniformArray>()[4] == 0.5f);
}

TEST_CASE( "Style Uniforms Parsing and Injection Test: textures uniform value", "[StyleUniforms][core][yaml]") {

    Node node = YAML::Load(R"END(
        u_tex : texture1.png
        u_tex2 : [texture2.png, texture3.png, texture4.png]
        )END");

    auto uniformValues = SceneLoader::parseStyleUniforms(node["u_tex"], scene);
    REQUIRE(uniformValues.value.is<std::string>());
    REQUIRE(uniformValues.value.get<std::string>() == "texture1.png");
    REQUIRE(uniformValues.type == "sampler2D");

    uniformValues = SceneLoader::parseStyleUniforms(node["u_tex2"], scene);
    REQUIRE(uniformValues.value.is<UniformTextureArray>());
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names.size() == 3);
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names[0] == "texture2.png");
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names[1] == "texture3.png");
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names[2] == "texture4.png");
}

