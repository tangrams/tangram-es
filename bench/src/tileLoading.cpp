#include "tangram.h"
#include "gl.h"
#include "platform_mock.h"
#include "log.h"
#include "data/tileSource.h"
#include "scene/sceneLoader.h"
#include "scene/scene.h"
#include "style/style.h"
#include "scene/styleContext.h"
#include "util/mapProjection.h"
#include "tile/tile.h"
#include "tile/tileBuilder.h"
#include "tile/tileTask.h"
#include "text/fontContext.h"

#include <vector>
#include <iostream>
#include <fstream>

#include "benchmark/benchmark_api.h"
#include "benchmark/benchmark.h"

using namespace Tangram;

struct TestContext {

    MercatorProjection s_projection;
    const char* sceneFile = "scene.yaml";

    std::shared_ptr<Platform> platform = std::make_shared<MockPlatform>();

    std::shared_ptr<Scene> scene;
    StyleContext styleContext;

    std::shared_ptr<TileSource> source;

    std::vector<char> rawTileData;

    std::shared_ptr<TileData> tileData;

    std::unique_ptr<TileBuilder> tileBuilder;

    void loadScene(const char* sceneFile) {
        scene = std::make_shared<Scene>(platform, sceneFile);
        auto sceneString = platform->stringFromFile(sceneFile);

        try { scene->config() = YAML::Load(sceneString); }
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
        std::ifstream resource(path, std::ifstream::ate | std::ifstream::binary);
        if(!resource.is_open()) {
            LOGE("Failed to read file at path: %s", path.c_str());
            return;
        }

        size_t _size = resource.tellg();
        resource.seekg(std::ifstream::beg);

        rawTileData.resize(_size);

        resource.read(&rawTileData[0], _size);
        resource.close();
    }

    void parseTile() {
        Tile tile({0,0,10,10,0}, s_projection);
        source = *scene->tileSources().begin();
        auto task = source->createTask(tile.getID());
        auto& t = dynamic_cast<BinaryTileTask&>(*task);
        t.rawTileData = std::make_shared<std::vector<char>>(rawTileData);

        tileData = source->parse(*task, s_projection);
    }
};

class TileLoadingFixture : public benchmark::Fixture {
public:
    TestContext ctx;
    MercatorProjection s_projection;
    const char* sceneFile = "scene.yaml";

    std::shared_ptr<Tile> result;

    void SetUp() override {
        LOG("SETUP");
        ctx.loadScene(sceneFile);
        ctx.loadTile("tile.mvt");
        ctx.parseTile();
        LOG("Ready");
    }
    void TearDown() override {
        result.reset();
        LOG("TEARDOWN");
    }
};

BENCHMARK_DEFINE_F(TileLoadingFixture, BuildTest)(benchmark::State& st) {
#if 0
    while (st.KeepRunning()) {
        ctx.parseTile();
        result = ctx.tileBuilder->build({0,0,10,10,0}, *ctx.tileData, *ctx.source);

        LOG("ok %d / bytes - %d", bool(result), result->getMemoryUsage());
    }
#endif
}

BENCHMARK_REGISTER_F(TileLoadingFixture, BuildTest);



BENCHMARK_MAIN();
