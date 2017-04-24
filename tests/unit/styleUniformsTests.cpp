#include "catch.hpp"

#include <iostream>
#include <vector>
#include <string>

#include "yaml-cpp/yaml.h"
#include "scene/filters.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"
#include "util/variant.h"
#include "platform_mock.h"

using namespace Tangram;
using YAML::Node;

TEST_CASE( "Style Uniforms Parsing and Injection Test: Float uniform value", "[StyleUniforms][core][yaml]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    std::shared_ptr<Scene> scene = std::make_shared<Scene>(platform);

    Node node = YAML::Load(R"END(
        u_float: 0.5
        )END");

    StyleUniform uniformValues;

    REQUIRE(SceneLoader::parseStyleUniforms(platform, node["u_float"], scene, uniformValues));
    REQUIRE(uniformValues.value.is<float>());
    REQUIRE(uniformValues.value.get<float>() == 0.5);
    REQUIRE(uniformValues.type == "float");
}

TEST_CASE( "Style Uniforms Parsing and Injection Test: Boolean uniform value", "[StyleUniforms][core][yaml]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    std::shared_ptr<Scene> scene = std::make_shared<Scene>(platform);

    Node node = YAML::Load(R"END(
        u_true: true
        u_false: false
        )END");

    StyleUniform uniformValues;

    REQUIRE(SceneLoader::parseStyleUniforms(platform, node["u_true"], scene, uniformValues));
    REQUIRE(uniformValues.value.is<bool>());
    REQUIRE(uniformValues.value.get<bool>() == 1);
    REQUIRE(uniformValues.type == "bool");

    REQUIRE(SceneLoader::parseStyleUniforms(platform, node["u_false"], scene, uniformValues));
    REQUIRE(uniformValues.value.is<bool>());
    REQUIRE(uniformValues.value.get<bool>() == 0);
    REQUIRE(uniformValues.type == "bool");
}

TEST_CASE( "Style Uniforms Parsing and Injection Test: vec2, vec3, vec4 uniform value", "[StyleUniforms][core][yaml]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    std::shared_ptr<Scene> scene = std::make_shared<Scene>(platform);

    Node node = YAML::Load(R"END(
        u_vec2: [0.1, 0.2]
        u_vec3: [0.1, 0.2, 0.3]
        u_vec4: [0.1, 0.2, 0.3, 0.4]
        u_array: [0.1, 0.2, 0.3, 0.4, 0.5]
        )END");

    StyleUniform uniformValues;

    REQUIRE(SceneLoader::parseStyleUniforms(platform, node["u_vec2"], scene, uniformValues));
    REQUIRE(uniformValues.value.is<glm::vec2>());
    REQUIRE(uniformValues.value.get<glm::vec2>().x == 0.1f);
    REQUIRE(uniformValues.value.get<glm::vec2>().y == 0.2f);
    REQUIRE(uniformValues.type == "vec2");

    REQUIRE(SceneLoader::parseStyleUniforms(platform, node["u_vec3"], scene, uniformValues));
    REQUIRE(uniformValues.value.is<glm::vec3>());
    REQUIRE(uniformValues.value.get<glm::vec3>().x == 0.1f);
    REQUIRE(uniformValues.value.get<glm::vec3>().y == 0.2f);
    REQUIRE(uniformValues.value.get<glm::vec3>().z == 0.3f);
    REQUIRE(uniformValues.type == "vec3");

    REQUIRE(SceneLoader::parseStyleUniforms(platform, node["u_vec4"], scene, uniformValues));
    REQUIRE(uniformValues.value.is<glm::vec4>());
    REQUIRE(uniformValues.value.get<glm::vec4>().x == 0.1f);
    REQUIRE(uniformValues.value.get<glm::vec4>().y == 0.2f);
    REQUIRE(uniformValues.value.get<glm::vec4>().z == 0.3f);
    REQUIRE(uniformValues.value.get<glm::vec4>().w == 0.4f);
    REQUIRE(uniformValues.type == "vec4");

    REQUIRE(SceneLoader::parseStyleUniforms(platform, node["u_array"], scene, uniformValues));
    REQUIRE(uniformValues.value.is<UniformArray1f>());
    REQUIRE(uniformValues.value.get<UniformArray1f>()[0] == 0.1f);
    REQUIRE(uniformValues.value.get<UniformArray1f>()[1] == 0.2f);
    REQUIRE(uniformValues.value.get<UniformArray1f>()[2] == 0.3f);
    REQUIRE(uniformValues.value.get<UniformArray1f>()[3] == 0.4f);
    REQUIRE(uniformValues.value.get<UniformArray1f>()[4] == 0.5f);
}

TEST_CASE( "Style Uniforms Parsing and Injection Test: textures uniform value", "[StyleUniforms][core][yaml]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    std::shared_ptr<Scene> scene = std::make_shared<Scene>(platform);

    Node node = YAML::Load(R"END(
        u_tex : img/cross.png
        u_tex2 : [img/cross.png, img/normals.jpg, img/sem.jpg]
        )END");

    StyleUniform uniformValues;

    REQUIRE(SceneLoader::parseStyleUniforms(platform, node["u_tex"], scene, uniformValues));
    REQUIRE(uniformValues.value.is<std::string>());
    REQUIRE(uniformValues.value.get<std::string>() == "img/cross.png");
    REQUIRE(uniformValues.type == "sampler2D");

    REQUIRE(SceneLoader::parseStyleUniforms(platform, node["u_tex2"], scene, uniformValues));
    REQUIRE(uniformValues.value.is<UniformTextureArray>());
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names.size() == 3);
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names[0] == "img/cross.png");
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names[1] == "img/normals.jpg");
    REQUIRE(uniformValues.value.get<UniformTextureArray>().names[2] == "img/sem.jpg");
}

TEST_CASE( "Style Uniforms Parsing failure Tests: textures uniform value", "[StyleUniforms][core][yaml]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    std::shared_ptr<Scene> scene = std::make_shared<Scene>(platform);

    Node node = YAML::Load(R"END(
        u_tex : not_a_texture
        u_tex2 : [not_a_texture_path2, not_a_texture_path_1]
        u_uniform_float0: 0.5f
        u_uniform_float1: 0s.5
        )END");

    StyleUniform uniformValues;

    REQUIRE(!SceneLoader::parseStyleUniforms(platform, node["u_tex"], scene, uniformValues));
    REQUIRE(!SceneLoader::parseStyleUniforms(platform, node["u_tex2"], scene, uniformValues));
    REQUIRE(!SceneLoader::parseStyleUniforms(platform, node["u_uniform_float0"], scene, uniformValues));
    REQUIRE(!SceneLoader::parseStyleUniforms(platform, node["u_uniform_float1"], scene, uniformValues));
}

