#include "catch.hpp"

#include "mockPlatform.h"
#include "scene/importer.h"

#include "yaml-cpp/yaml.h"

#include <iostream>
#include <vector>

using namespace Tangram;
using namespace YAML;

struct ImportMockPlatform : public MockPlatform {
    ImportMockPlatform() {

    putMockUrlContents("/root/a.yaml", R"END(
        import: b.yaml
        value: a
        has_a: true
    )END");

    putMockUrlContents("/root/b.yaml", R"END(
        value: b
        has_b: true
    )END");

    putMockUrlContents("/root/c.yaml", R"END(
        import: [a.yaml, b.yaml]
        value: c
        has_c: true
    )END");

    putMockUrlContents("/root/cycle_simple.yaml", R"END(
        import: cycle_simple.yaml
        value: cyclic
    )END");

    putMockUrlContents("/root/cycle_tricky.yaml", R"END(
        import: imports/cycle_tricky.yaml
        has_cycle_tricky: true
    )END");

    putMockUrlContents("/root/imports/cycle_tricky.yaml", R"END(
        import: ../cycle_tricky.yaml
        has_imports_cycle_tricky: true
    )END");

    putMockUrlContents("/root/urls.yaml", R"END(
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
    )END");

    putMockUrlContents("/root/imports/urls.yaml", R"END(
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
    )END");

    putMockUrlContents("/root/globals.yaml", R"END(
        fonts: { aFont: { url: global.fontUrl } }
        sources: { aSource: { url: global.sourceUrl } }
        textures: { aTexture: { url: global.textureUrl } }
        styles: { aStyle: { texture: global.textureUrl, shaders: { uniforms: { aUniform: global.textureUrl } } } }
    )END");
    }
};

                           
std::shared_ptr<Scene> getScene(MockPlatform& platform, const Url& url) {
    return std::make_shared<Scene>(platform, url);
}

TEST_CASE("Imported scenes are merged with the parent scene", "[import][core]") {
    ImportMockPlatform platform;
    Importer importer(getScene(platform, Url("/root/a.yaml")));
    auto root = importer.applySceneImports(platform);

    CHECK(root["value"].Scalar() == "a");
    CHECK(root["has_a"].Scalar() == "true");
    CHECK(root["has_b"].Scalar() == "true");
}

TEST_CASE("Nested imports are merged recursively", "[import][core]") {
    ImportMockPlatform platform;
    Importer importer(getScene(platform, Url("/root/c.yaml")));
    auto root = importer.applySceneImports(platform);

    CHECK(root["value"].Scalar() == "c");
    CHECK(root["has_a"].Scalar() == "true");
    CHECK(root["has_b"].Scalar() == "true");
    CHECK(root["has_c"].Scalar() == "true");
}

TEST_CASE("Imports that would start a cycle are ignored", "[import][core]") {
    ImportMockPlatform platform;
    Importer importer(getScene(platform, Url("/root/cycle_simple.yaml")));

    // If import cycles aren't checked for and stopped, this call won't return.
    auto root = importer.applySceneImports(platform);

    // Check that the scene values were applied.
    CHECK(root["value"].Scalar() == "cyclic");
}

TEST_CASE("Tricky import cycles are ignored", "[import][core]") {
    ImportMockPlatform platform;
    Importer importer(getScene(platform, Url("/root/cycle_tricky.yaml")));

    // The nested import should resolve to the same path as the original file,
    // and so the importer should break the cycle.
    auto root = importer.applySceneImports(platform);

    // Check that the imported scene values were merged.
    CHECK(root["has_cycle_tricky"].Scalar() == "true");
    CHECK(root["has_imports_cycle_tricky"].Scalar() == "true");
}

TEST_CASE("Scene URLs are resolved against their parent during import", "[import][core]") {
    ImportMockPlatform platform;
    Importer importer(getScene(platform, Url("/root/urls.yaml")));
    auto root = importer.applySceneImports(platform);

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
    ImportMockPlatform platform;
    Importer importer(getScene(platform, Url("/root/globals.yaml")));
    auto root = importer.applySceneImports(platform);

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
    ImportMockPlatform platform;
    platform.putMockUrlContents("/base.yaml", R"END(
        import: [roads.yaml, roads-labels.yaml]
    )END");

    platform.putMockUrlContents("/roads.yaml", R"END(
            filter:
                - kind: highway
                - $zoom: { min: 8 }
    )END");

    platform.putMockUrlContents("/roads-labels.yaml", R"END(
                filter: { kind: highway }
    )END");

    Importer importer(getScene(platform, Url("/base.yaml")));
    auto root = importer.applySceneImports(platform);

    CHECK(root["filter"].IsMap());
    CHECK(root["filter"].size() == 1);
    CHECK(root["filter"]["kind"].Scalar() == "highway");
}

TEST_CASE("Sequence overwrites map", "[import][core]") {
    MockPlatform platform;
    platform.putMockUrlContents("/base.yaml", R"END(
        import: [map.yaml, sequence.yaml]
    )END");
    platform.putMockUrlContents("/map.yaml", R"END(
            a: { b: c }
    )END");

    platform.putMockUrlContents("/sequence.yaml", R"END(
            a: [ b, c]
    )END");

    Importer importer(getScene(platform, Url("/base.yaml")));
    auto root = importer.applySceneImports(platform);

    CHECK(root["a"].IsSequence());
    CHECK(root["a"].size() == 2);
}

TEST_CASE("Scalar and null overwrite correctly", "[import][core]") {
    MockPlatform platform;
    platform.putMockUrlContents("/base.yaml", R"END(
        import: [scalar.yaml, null.yaml]
        scalar_at_end: scalar
        null_at_end: null
    )END");
    platform.putMockUrlContents("/scalar.yaml", R"END(
            null_at_end: scalar
    )END");

    platform.putMockUrlContents("/null.yaml", R"END(
            scalar_at_end: null
    )END");

    Importer importer(getScene(platform, Url("/base.yaml")));
    auto root = importer.applySceneImports(platform);

    CHECK(root["scalar_at_end"].Scalar() == "scalar");
    CHECK(root["null_at_end"].IsNull());
}

TEST_CASE("Scene load from source string", "[import][core]") {
    MockPlatform platform;
    std::unordered_map<Url, std::string> testScenes;
    platform.putMockUrlContents("/resource_root/scalar.yaml", R"END(
            null_at_end: scalar
    )END");
    platform.putMockUrlContents("/resource_root/null.yaml", R"END(
            scalar_at_end: null
    )END");

    std::string base_yaml = R"END(
        import: [scalar.yaml, null.yaml]
        scalar_at_end: scalar
        null_at_end: null
    )END";

    auto scene = std::make_shared<Scene>(platform, base_yaml, Url("/resource_root/"));

    Importer importer(scene);
    
    auto root = importer.applySceneImports(platform);

    CHECK(root["scalar_at_end"].Scalar() == "scalar");
    CHECK(root["null_at_end"].IsNull());
}
