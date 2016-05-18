#include "catch.hpp"

#include <iostream>
#include <vector>

#include "yaml-cpp/yaml.h"
#include "scene/importer.h"

using namespace Tangram;
using namespace YAML;

class TestImporter : public Importer {

public:
    TestImporter();

protected:
    // The only thing different here is the call to `getSceneString` instead of reading and
    // loading a yaml file, which is in the parent importer class
    virtual bool loadScene(const std::string& scenePath) override;

private:
    std::string getSceneString(const std::string& scenePath) {
        return m_testScenes[scenePath];
    }

    std::unordered_map<std::string, std::string> m_testScenes;
};

TestImporter::TestImporter() {
    // Set multiple scenes here, basically set m_testScenes

    m_testScenes["import/vehicle.yaml"] = std::string(R"END(
                                name: vehicle
                                type: vehicle
                                category: vehicle
                                )END");

    m_testScenes["car.yaml"] = std::string(R"END(
                                import: import/vehicle.yaml
                                type: car
                                )END");

    m_testScenes["thisCar.yaml"] = std::string(R"END(
                                    import: [import/vehicle.yaml, car.yaml]
                                    name: thisCar
                                    )END");

    m_testScenes["/absolutePath/thisCar.yaml"] = std::string(R"END(
                                    import: [import/vehicle.yaml, car.yaml]
                                    name: thisCar
                                    )END");


    // All about textures
    m_testScenes["import/import.yaml"] = std::string(
            R"END(
                textures:
                    tex1: { url: "../resources/icons/poi.png" }
                    tex4: { url: "../resources/icons/poi2.png" }
                    tex5: { url: "importResources/tex.png" }
                    tex6: { url: "/absPath/tex.png" }
                styles:
                    styleA:
                        shaders:
                            uniforms:
                                u_tex3: "importResources/tex.png"
            )END");
    m_testScenes["scene.yaml"] = std::string(
            R"END(
                import: import/import.yaml
                textures:
                    tex1: { url: "resources/icons/poi.png" }
                    tex2: { url: "/absoluteTexPath/texture.png" }
                    tex3: { url: "resources/tex/sameName.png" }
                styles:
                    styleA:
                        texture: "resources/tex/sameName.png"
                        shaders:
                            uniforms:
                                u_tex1: "resources/tex/sameName.png"
                                u_tex2: ["uTex1.png", "resources/uTex2.png"]
            )END");
}

bool TestImporter::loadScene(const std::string& path) {
    auto scenePath = path;
    auto sceneString = getSceneString(scenePath); // Importer.cpp read the scene from the file
    auto sceneName = getFilename(scenePath);

    if (m_scenes.find(sceneName) != m_scenes.end()) { return true; }

    if (m_scenes.size() == 0 && scenePath[0] == '/') { scenePath = getFilename(scenePath); }

    try {
        auto root = YAML::Load(sceneString);
        normalizeSceneImports(root, scenePath);
        normalizeSceneTextures(root, scenePath);
        auto imports = getScenesToImport(root);
        m_scenes[sceneName] = root;
        for (const auto& import : imports) {
            // TODO: What happens when parsing fails for an import
            loadScene(import);
        }
    }
    catch (YAML::ParserException e) {
        return false;
    }
    return true;
}

TEST_CASE( "Basic importing - property overriding", "[scene-import][core]") {

    TestImporter importer;
    auto root = importer.applySceneImports("thisCar.yaml");

    REQUIRE(root["import"].Scalar() == "");
    REQUIRE(root["name"].Scalar() == "thisCar");
    REQUIRE(root["type"].Scalar() == "car");
    REQUIRE(root["category"].Scalar() == "vehicle");

}

TEST_CASE( "Basic importing with absolute path scene file - property overriding", "[scene-import][core]") {
    TestImporter importer;
    auto root = importer.applySceneImports("/absolutePath/thisCar.yaml");

    REQUIRE(root["import"].Scalar() == "");
    REQUIRE(root["name"].Scalar() == "thisCar");
    REQUIRE(root["type"].Scalar() == "car");
    REQUIRE(root["category"].Scalar() == "vehicle");
}

TEST_CASE( "Texture path tests after importing process", "[scene-import][core]") {
    TestImporter importer;
    auto root = importer.applySceneImports("scene.yaml");

    REQUIRE(root["import"].Scalar() == "");
    const auto& texturesNode = root["textures"];
    REQUIRE(texturesNode["tex1"]["url"].Scalar() == "resources/icons/poi.png");
    REQUIRE(texturesNode["tex2"]["url"].Scalar() == "/absoluteTexPath/texture.png");
    REQUIRE(texturesNode["tex3"]["url"].Scalar() == "resources/tex/sameName.png");
    REQUIRE(texturesNode["tex4"]["url"].Scalar() == "import/../resources/icons/poi2.png");
    REQUIRE(texturesNode["tex5"]["url"].Scalar() == "import/importResources/tex.png");
    REQUIRE(texturesNode["tex6"]["url"].Scalar() == "/absPath/tex.png");

    const auto& styleNode = root["styles"]["styleA"];
    const auto& uniformsNode = styleNode["shaders"]["uniforms"];
    REQUIRE(styleNode["texture"].Scalar() == "resources/tex/sameName.png");
    REQUIRE(uniformsNode["u_tex1"].Scalar() == "resources/tex/sameName.png");
    REQUIRE(uniformsNode["u_tex2"][0].Scalar() == "uTex1.png");
    REQUIRE(uniformsNode["u_tex2"][1].Scalar() == "resources/uTex2.png");
    REQUIRE(uniformsNode["u_tex3"].Scalar() == "import/importResources/tex.png");
}
