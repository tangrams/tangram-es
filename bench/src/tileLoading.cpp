#include "data/tileSource.h"
#include "gl.h"
#include "log.h"
#include "map.h"
#include "mockPlatform.h"
#include "scene/importer.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "scene/styleContext.h"
#include "style/style.h"
#include "tile/tile.h"
#include "tile/tileBuilder.h"
#include "tile/tileTask.h"
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
    StyleContext styleContext;

    std::shared_ptr<TileSource> source;

    std::vector<char> rawTileData;

    std::shared_ptr<TileData> tileData;

    std::unique_ptr<TileBuilder> tileBuilder;

    void loadScene(const char* path) {

        Url sceneUrl(path);
        platform->putMockUrlContents(sceneUrl, MockPlatform::getBytesFromFile(path));

        scene = std::make_shared<Scene>(platform, sceneUrl);
        Importer importer(scene);

        try {
            scene->config() = importer.applySceneImports(platform);
        }
        catch (YAML::ParserException e) {
            LOGE("Parsing scene config '%s'", e.what());
            return;
        }
        SceneLoader::applyConfig(platform, scene);

        scene->fontContext()->loadFonts();

        styleContext.initFunctions(*scene);
        styleContext.setKeywordZoom(0);

        source = *scene->tileSources().begin();
        tileBuilder = std::make_unique<TileBuilder>(scene);
    }

    void loadTile(const char* path){

        rawTileData = MockPlatform::getBytesFromFile(path);

    }

    void parseTile() {
        Tile tile({0,0,10,10});
        source = *scene->tileSources().begin();
        auto task = source->createTask(tile.getID());
        auto& t = dynamic_cast<BinaryTileTask&>(*task);
        t.rawTileData = std::make_shared<std::vector<char>>(rawTileData);

        tileData = source->parse(*task);
    }
};

class TileLoadingFixture : public benchmark::Fixture {
public:
    TestContext ctx;
    std::shared_ptr<Tile> result;

    void SetUp() override {
        LOG("SETUP");
        ctx.loadScene("scene.yaml");
        ctx.loadTile("tile.mvt");
        LOG("READY");
    }
    void TearDown() override {
        result.reset();
        LOG("TEARDOWN");
    }
};

BENCHMARK_DEFINE_F(TileLoadingFixture, BuildTest)(benchmark::State& st) {
    while (st.KeepRunning()) {
        ctx.parseTile();
        if (!ctx.tileData) { break; }

        result = ctx.tileBuilder->build({0,0,10,10}, *ctx.tileData, *ctx.source);

        LOG("ok %d / bytes - %d", bool(result), result->getMemoryUsage());
    }
}

BENCHMARK_REGISTER_F(TileLoadingFixture, BuildTest);



BENCHMARK_MAIN();
