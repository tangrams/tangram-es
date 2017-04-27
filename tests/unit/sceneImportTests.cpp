#include "catch.hpp"

#include <iostream>
#include <vector>

#include "yaml-cpp/yaml.h"
#include "platform_mock.h"
#include "scene/importer.h"

using namespace Tangram;
using namespace YAML;

class TestImporter : public Importer {

public:
    TestImporter();

    TestImporter(std::unordered_map<Url, std::string> _testScenes) : m_testScenes(_testScenes) {}

protected:
    virtual std::string getSceneString(const std::shared_ptr<Platform>& platform, const Url& scenePath) override {
        return m_testScenes[scenePath];
    }

    std::unordered_map<Url, std::string> m_testScenes;
};

TestImporter::TestImporter() {

    m_testScenes["/root/a.yaml"] = R"END(
        import: b.yaml
        value: a
        has_a: true
    )END";

    m_testScenes["/root/b.yaml"] = R"END(
        value: b
        has_b: true
    )END";

    m_testScenes["/root/c.yaml"] = R"END(
        import: [a.yaml, b.yaml]
        value: c
        has_c: true
    )END";

    m_testScenes["/root/cycle_simple.yaml"] = R"END(
        import: cycle_simple.yaml
        value: cyclic
    )END";

    m_testScenes["/root/cycle_tricky.yaml"] = R"END(
        import: imports/cycle_tricky.yaml
        has_cycle_tricky: true
    )END";

    m_testScenes["/root/imports/cycle_tricky.yaml"] = R"END(
        import: ../cycle_tricky.yaml
        has_imports_cycle_tricky: true
    )END";

    m_testScenes["/root/urls.yaml"] = R"END(
        import: imports/urls.yaml
        fonts: { fontA: { url: https://host/font.woff } }
        sources: { sourceA: { url: 'https://host/tiles/{z}/{y}/{x}.mvt' } }
        textures:
            tex1: { url: "path/to/texture.png" }
            tex2: { url: "../up_a_directory.png" }
        styles:
            styleA:
                texture: "path/to/texture.png"
                shaders:
                    uniforms:
                        u_tex1: "/at_root.png"
                        u_tex2: ["path/to/texture.png", tex2]
                        u_tex3: tex3
                        u_bool: true
                        u_float: 0.25
    )END";

    m_testScenes["/root/imports/urls.yaml"] = R"END(
        fonts: { fontB: [ { url: fonts/0.ttf }, { url: fonts/1.ttf } ] }
        sources: { sourceB: { url: "tiles/{z}/{y}/{x}.mvt" } }
        textures:
            tex3: { url: "in_imports.png" }
            tex4: { url: "../not_in_imports.png" }
            tex5: { url: "/at_root.png" }
        styles:
            styleB:
                texture: "in_imports.png"
                shaders:
                    uniforms:
                        u_tex1: "in_imports.png"
                        u_tex2: tex2
    )END";

    m_testScenes["/root/globals.yaml"] = R"END(
        fonts: { aFont: { url: global.fontUrl } }
        sources: { aSource: { url: global.sourceUrl } }
        textures: { aTexture: { url: global.textureUrl } }
        styles: { aStyle: { texture: global.textureUrl, shaders: { uniforms: { aUniform: global.textureUrl } } } }
    )END";
}

TEST_CASE("Imported scenes are merged with the parent scene", "[import][core]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    TestImporter importer;
    auto root = importer.applySceneImports(platform, "a.yaml", "/root/");

    CHECK(root["value"].Scalar() == "a");
    CHECK(root["has_a"].Scalar() == "true");
    CHECK(root["has_b"].Scalar() == "true");
}

TEST_CASE("Nested imports are merged recursively", "[import][core]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    TestImporter importer;
    auto root = importer.applySceneImports(platform, "c.yaml", "/root/");

    CHECK(root["value"].Scalar() == "c");
    CHECK(root["has_a"].Scalar() == "true");
    CHECK(root["has_b"].Scalar() == "true");
    CHECK(root["has_c"].Scalar() == "true");
}

TEST_CASE("Imports that would start a cycle are ignored", "[import][core]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    TestImporter importer;

    // If import cycles aren't checked for and stopped, this call won't return.
    auto root = importer.applySceneImports(platform, "cycle_simple.yaml", "/root/");

    // Check that the scene values were applied.
    CHECK(root["value"].Scalar() == "cyclic");
}

TEST_CASE("Tricky import cycles are ignored", "[import][core]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    TestImporter importer;

    // The nested import should resolve to the same path as the original file,
    // and so the importer should break the cycle.
    auto root = importer.applySceneImports(platform, "cycle_tricky.yaml", "/root/");

    // Check that the imported scene values were merged.
    CHECK(root["has_cycle_tricky"].Scalar() == "true");
    CHECK(root["has_imports_cycle_tricky"].Scalar() == "true");
}

TEST_CASE("Scene URLs are resolved against their parent during import", "[import][core]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    TestImporter importer;
    auto root = importer.applySceneImports(platform, "urls.yaml", "/root/");

    // Check that global texture URLs are resolved correctly.

    auto textures = root["textures"];

    CHECK(textures["tex1"]["url"].Scalar() == "/root/path/to/texture.png");
    CHECK(textures["tex2"]["url"].Scalar() == "/up_a_directory.png");
    CHECK(textures["tex3"]["url"].Scalar() == "/root/imports/in_imports.png");
    CHECK(textures["tex4"]["url"].Scalar() == "/root/not_in_imports.png");
    CHECK(textures["tex5"]["url"].Scalar() == "/at_root.png");

    // Check that "inline" texture URLs are resolved correctly.

    auto styleA = root["styles"]["styleA"];

    CHECK(styleA["texture"].Scalar() == "/root/path/to/texture.png");

    auto uniformsA = styleA["shaders"]["uniforms"];

    CHECK(uniformsA["u_tex1"].Scalar() == "/at_root.png");
    CHECK(uniformsA["u_tex2"][0].Scalar() == "/root/path/to/texture.png");
    CHECK(uniformsA["u_tex2"][1].Scalar() == "tex2");
    CHECK(uniformsA["u_bool"].Scalar() == "true");
    CHECK(uniformsA["u_float"].Scalar() == "0.25");
    CHECK(uniformsA["u_tex3"].Scalar() == "tex3");

    auto styleB = root["styles"]["styleB"];

    CHECK(styleB["texture"].Scalar() == "/root/imports/in_imports.png");

    auto uniformsB = styleB["shaders"]["uniforms"];

    CHECK(uniformsB["u_tex1"].Scalar() == "/root/imports/in_imports.png");
    // Don't use global textures from importing scene
    CHECK(uniformsB["u_tex2"].Scalar() == "/root/imports/tex2");

    // Check that data source URLs are resolved correctly.

    CHECK(root["sources"]["sourceA"]["url"].Scalar() == "https://host/tiles/{z}/{y}/{x}.mvt");
    CHECK(root["sources"]["sourceB"]["url"].Scalar() == "/root/imports/tiles/{z}/{y}/{x}.mvt");

    // Check that font URLs are resolved correctly.

    CHECK(root["fonts"]["fontA"]["url"].Scalar() == "https://host/font.woff");
    CHECK(root["fonts"]["fontB"][0]["url"].Scalar() == "/root/imports/fonts/0.ttf");
    CHECK(root["fonts"]["fontB"][1]["url"].Scalar() == "/root/imports/fonts/1.ttf");

    // We don't explicitly check that import URLs are resolved correctly because if they were not,
    // the scenes wouldn't be loaded and merged; i.e. we already test it implicitly.
}

TEST_CASE("References to globals are not treated like URLs during importing", "[import][core]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    TestImporter importer;
    auto root = importer.applySceneImports(platform, "globals.yaml", "/root/");

    // Check that font global references are preserved.
    CHECK(root["fonts"]["aFont"]["url"].Scalar() == "global.fontUrl");

    // Check that data source global references are preserved.
    CHECK(root["sources"]["aSource"]["url"].Scalar() == "global.sourceUrl");

    // Check that texture global references are preserved.
    CHECK(root["textures"]["aTexture"]["url"].Scalar() == "global.textureUrl");
    CHECK(root["styles"]["aStyle"]["texture"].Scalar() == "global.textureUrl");
    CHECK(root["styles"]["aStyle"]["shaders"]["uniforms"]["aUniform"].Scalar() == "global.textureUrl");
}

TEST_CASE("Map overwrites sequence", "[import][core]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    std::unordered_map<Url, std::string> testScenes;
    testScenes["/base.yaml"] = R"END(
        import: [roads.yaml, roads-labels.yaml]
    )END";

    testScenes["/roads.yaml"] = R"END(
            filter:
                - kind: highway
                - $zoom: { min: 8 }
    )END";

    testScenes["/roads-labels.yaml"] = R"END(
                filter: { kind: highway }
    )END";

    TestImporter importer(testScenes);
    auto root = importer.applySceneImports(platform, "base.yaml", "/");

    CHECK(root["filter"].IsMap());
    CHECK(root["filter"].size() == 1);
    CHECK(root["filter"]["kind"].Scalar() == "highway");
}

TEST_CASE("Sequence overwrites map", "[import][core]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    std::unordered_map<Url, std::string> testScenes;
    testScenes["/base.yaml"] = R"END(
        import: [map.yaml, sequence.yaml]
    )END";
    testScenes["/map.yaml"] = R"END(
            a: { b: c }
    )END";

    testScenes["/sequence.yaml"] = R"END(
            a: [ b, c]
    )END";

    TestImporter importer(testScenes);
    auto root = importer.applySceneImports(platform, "base.yaml", "/");

    CHECK(root["a"].IsSequence());
    CHECK(root["a"].size() == 2);
}

TEST_CASE("Scalar and null overwrite correctly", "[import][core]") {
    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();
    std::unordered_map<Url, std::string> testScenes;
    testScenes["/base.yaml"] = R"END(
        import: [scalar.yaml, null.yaml]
        scalar_at_end: scalar
        null_at_end: null
    )END";
    testScenes["/scalar.yaml"] = R"END(
            null_at_end: scalar
    )END";

    testScenes["/null.yaml"] = R"END(
            scalar_at_end: null
    )END";

    TestImporter importer(testScenes);
    auto root = importer.applySceneImports(platform, "base.yaml", "/");

    CHECK(root["scalar_at_end"].Scalar() == "scalar");
    CHECK(root["null_at_end"].IsNull());
}
