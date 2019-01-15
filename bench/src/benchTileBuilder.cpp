#include "benchmark/benchmark.h"

#include "data/tileSource.h"
#include "gl.h"
#include "log.h"
#include "map.h"
#include "mockPlatform.h"
#include "scene/importer.h"
#include "scene/scene.h"
#include "scene/sceneLoader.h"
#include "style/style.h"
#include "text/fontContext.h"
#include "tile/tile.h"
#include "tile/tileBuilder.h"
#include "tile/tileTask.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#define NUM_ITERATIONS 0

#if NUM_ITERATIONS
#define ITERATIONS ->Iterations(NUM_ITERATIONS)
#else
#define ITERATIONS
#endif

#define RUN(FIXTURE, NAME)                                              \
    BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& st) { while (st.KeepRunning()) { run(); } } \
    BENCHMARK_REGISTER_F(FIXTURE, NAME)ITERATIONS;

using namespace Tangram;

//const char scene_file[] = "bubble-wrap-style.zip";
const char scene_file[] = "res/scene.yaml";
const char tile_file[] = "res/tile.mvt";

std::shared_ptr<Scene> scene;
std::shared_ptr<TileSource> source;
std::shared_ptr<TileData> tileData;
std::shared_ptr<MockPlatform> platform;

void globalSetup() {
    static std::atomic<bool> initialized{false};
    if (initialized.exchange(true)) { return; }

    platform = std::make_shared<MockPlatform>();

    Url sceneUrl(scene_file);
    scene = std::make_shared<Scene>(platform, sceneUrl);
    Importer importer(scene);
    try {
        scene->config() = importer.applySceneImports(platform);
    }
    catch (const YAML::ParserException& e) {
        LOGE("Parsing scene config '%s'", e.what());
        exit(-1);
    }
    if (!scene->config()) {
        LOGE("Invalid scene file '%s'", scene_file);
        exit(-1);
    }
    SceneLoader::applyConfig(platform, scene);
    scene->fontContext()->loadFonts();

    for (auto& s : scene->tileSources()) {
        if (s->generateGeometry()) {
            source = s;
            break;
        }
    }
    if (!source) {
        LOGE("No TileSource found");
        exit(-1);
    }

    while (scene->pendingFonts || scene->pendingFonts) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }

    Tile tile({0,0,10,10});
    auto task = source->createTask(tile.getID());
    auto& t = dynamic_cast<BinaryTileTask&>(*task);

    auto rawTileData = MockPlatform::getBytesFromFile(tile_file);
    t.rawTileData = std::make_shared<std::vector<char>>(rawTileData);
    tileData = source->parse(*task);
    if (!tileData) {
        LOGE("Invalid tile file '%s'", tile_file);
        exit(-1);
    }
}

class TileBuilderFixture : public benchmark::Fixture {
public:
    std::unique_ptr<TileBuilder> tileBuilder;
    StyleContext* styleContext;
    std::shared_ptr<Tile> result;
    void SetUp(const ::benchmark::State& state) override {
        globalSetup();
        styleContext = new StyleContext();
        tileBuilder = std::make_unique<TileBuilder>(scene, styleContext);
    }
    void TearDown(const ::benchmark::State& state) override {
        result.reset();
    }

    __attribute__ ((noinline)) void run() {
        result = tileBuilder->build({0,0,10,10}, *tileData, *source);
        styleContext->clear();
    }
};

RUN(TileBuilderFixture, TileBuilderBench);



BENCHMARK_MAIN();
