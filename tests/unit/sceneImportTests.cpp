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
    // TODO: scenes to test texture importing
}

bool TestImporter::loadScene(const std::string& path) {
    auto scenePath = path;
    auto sceneString = getSceneString(scenePath); // Importer.cpp read the scene from the file
    auto sceneName = getFilename(scenePath);

    if (m_scenes.find(sceneName) != m_scenes.end()) { return true; }

    // Make sure all references from uber scene file are relative to itself, instead of being
    // absolute paths (Example: when loading a file using command line args).
    // TODO: Could be made better later.
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
        //LOGE("Parsing scene config '%s'", e.what());
        return false;
    }
    return true;
}

TEST_CASE( "Basic importing - property overriding", "[scene-import][core]") {

    TestImporter importer;
    auto root = importer.applySceneImports("thisCar.yaml");

    REQUIRE(root["name"].Scalar() == "thisCar");
    REQUIRE(root["type"].Scalar() == "car");
    REQUIRE(root["category"].Scalar() == "vehicle");

    // see if imports worked ... order
    // see if texture urls are good
    // what happens to texture urls which are common between scenes
    //  - common names
    //  - common urls
}

TEST_CASE( "Basic importing with absolute path scene file - property overriding", "[scene-import][core]") {
    TestImporter importer;
    auto root = importer.applySceneImports("/absolutePath/thisCar.yaml");
    REQUIRE(root["name"].Scalar() == "thisCar");
    REQUIRE(root["type"].Scalar() == "car");
    REQUIRE(root["category"].Scalar() == "vehicle");
}
