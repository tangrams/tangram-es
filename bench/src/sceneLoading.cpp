#include "log.h"
#include "mockPlatform.h"
#include "scene/importer.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "text/fontContext.h"

#include <fstream>
#include <iostream>
#include <vector>

#include "benchmark/benchmark_api.h"
#include "benchmark/benchmark.h"

using namespace Tangram;

struct TestContext {

    std::shared_ptr<MockPlatform> platform = std::make_shared<MockPlatform>();

    std::shared_ptr<Scene> scene;

    void setupScene(const char* path) {
        Url sceneUrl(path);
        platform->putMockUrlContents(sceneUrl, MockPlatform::getBytesFromFile(path));
        scene = std::make_shared<Scene>(platform, sceneUrl);
    }

    void loadScene() {
        Importer importer(scene);
        try {
            scene->config() = importer.applySceneImports(platform);
        }
        catch (YAML::ParserException e) {
            LOGE("Parsing scene config '%s'", e.what());
            return;
        }
        scene->fontContext()->loadFonts();
        SceneLoader::applyConfig(platform, scene);
    }
};

class SceneLoadingFixture: public benchmark::Fixture {
public:
    TestContext ctx;

    void SetUp() override {
    }

    void TearDown() override {
        ctx.scene.reset();
    }
};

BENCHMARK_DEFINE_F(SceneLoadingFixture, BuildTest)(benchmark::State& st) {
    while (st.KeepRunning()) {
        ctx.setupScene("scene.yaml");
        ctx.loadScene();
    }
}
BENCHMARK_REGISTER_F(SceneLoadingFixture, BuildTest);

BENCHMARK_MAIN();
