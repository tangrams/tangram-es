#include "catch.hpp"

#include "log.h"
#include "mockPlatform.h"
#include "scene/filters.h"
#include "scene/importer.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "util/variant.h"

#include "yaml-cpp/yaml.h"

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

using namespace Tangram;
using YAML::Node;

TEST_CASE( "Style Uniforms Parsing and Injection Test: Float uniform value", "[StyleUniforms][core][yaml]") {

    Node node = YAML::Load(R"END(
        u_float: 0.5
        )END");

    StyleUniform uniformValues;
    SceneTextures textures;
    REQUIRE(SceneLoader::parseStyleUniforms(node["u_float"], uniformValues, textures));
    REQUIRE(uniformValues.value.is<float>());
    REQUIRE(uniformValues.value.get<float>() == 0.5);
    REQUIRE(uniformValues.type == "float");
}

TEST_CASE( "Style Uniforms Parsing and Injection Test: Boolean uniform value", "[StyleUniforms][core][yaml]") {

    Node node = YAML::Load(R"END(
        u_true: true
        u_false: false
        )END");

    StyleUniform uniformValues;
    SceneTextures textures;
    REQUIRE(SceneLoader::parseStyleUniforms(node["u_true"], uniformValues, textures));
    REQUIRE(uniformValues.value.is<bool>());
    REQUIRE(uniformValues.value.get<bool>() == 1);
    REQUIRE(uniformValues.type == "bool");

    REQUIRE(SceneLoader::parseStyleUniforms(node["u_false"], uniformValues, textures));
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

    StyleUniform uniformValues;
    SceneTextures textures;

    REQUIRE(SceneLoader::parseStyleUniforms(node["u_vec2"], uniformValues, textures));
    REQUIRE(uniformValues.value.is<glm::vec2>());
    REQUIRE(uniformValues.value.get<glm::vec2>().x == 0.1f);
    REQUIRE(uniformValues.value.get<glm::vec2>().y == 0.2f);
    REQUIRE(uniformValues.type == "vec2");

    REQUIRE(SceneLoader::parseStyleUniforms(node["u_vec3"], uniformValues, textures));
    REQUIRE(uniformValues.value.is<glm::vec3>());
    REQUIRE(uniformValues.value.get<glm::vec3>().x == 0.1f);
    REQUIRE(uniformValues.value.get<glm::vec3>().y == 0.2f);
    REQUIRE(uniformValues.value.get<glm::vec3>().z == 0.3f);
    REQUIRE(uniformValues.type == "vec3");

    REQUIRE(SceneLoader::parseStyleUniforms(node["u_vec4"], uniformValues, textures));
    REQUIRE(uniformValues.value.is<glm::vec4>());
    REQUIRE(uniformValues.value.get<glm::vec4>().x == 0.1f);
    REQUIRE(uniformValues.value.get<glm::vec4>().y == 0.2f);
    REQUIRE(uniformValues.value.get<glm::vec4>().z == 0.3f);
    REQUIRE(uniformValues.value.get<glm::vec4>().w == 0.4f);
    REQUIRE(uniformValues.type == "vec4");

    REQUIRE(SceneLoader::parseStyleUniforms(node["u_array"], uniformValues, textures));
    REQUIRE(uniformValues.value.is<UniformArray1f>());
    REQUIRE(uniformValues.value.get<UniformArray1f>()[0] == 0.1f);
    REQUIRE(uniformValues.value.get<UniformArray1f>()[1] == 0.2f);
    REQUIRE(uniformValues.value.get<UniformArray1f>()[2] == 0.3f);
    REQUIRE(uniformValues.value.get<UniformArray1f>()[3] == 0.4f);
    REQUIRE(uniformValues.value.get<UniformArray1f>()[4] == 0.5f);
}

TEST_CASE( "Style Uniforms Parsing and Injection Test: textures uniform value", "[StyleUniforms][core][yaml]") {

    Node node = YAML::Load(R"END(
        u_tex: img/cross.png
        u_tex2: [img/cross.png, img/normals.jpg, img/sem.jpg]
    )END");

    StyleUniform uniformValues;
    SceneTextures textures;

    REQUIRE(SceneLoader::parseStyleUniforms(node["u_tex"], uniformValues, textures));
    REQUIRE(uniformValues.value.is<std::string>());
    REQUIRE(uniformValues.value.get<std::string>() == "img/cross.png");
    REQUIRE(uniformValues.type == "sampler2D");

    REQUIRE(SceneLoader::parseStyleUniforms(node["u_tex2"], uniformValues, textures));
    REQUIRE(uniformValues.value.is<UniformTextureArray>());
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names.size() == 3);
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names[0] == "img/cross.png");
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names[1] == "img/normals.jpg");
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names[2] == "img/sem.jpg");
}
