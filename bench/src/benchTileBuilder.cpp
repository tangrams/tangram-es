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


#define RUN(FIXTURE, NAME)                                              \
    BENCHMARK_DEFINE_F(FIXTURE, NAME)(benchmark::State& st) { while (st.KeepRunning()) { run(); } } \
    BENCHMARK_REGISTER_F(FIXTURE, NAME);  //->Iterations(1)

using namespace Tangram;

//const char scene_file[] = "bubble-wrap-style.zip";
const char scene_file[] = "res/scene.yaml";
const char tile_file[] = "res/tile.mvt";

std::shared_ptr<Scene> scene;
std::shared_ptr<TileSource> source;
std::shared_ptr<TileData> tileData;

std::unique_ptr<MockPlatform> platform = std::make_unique<MockPlatform>();

void globalSetup() {
    static std::atomic<bool> initialized{false};
    if (initialized.exchange(true)) { return; }

    Url sceneUrl(scene_file);
    platform->putMockUrlContents(sceneUrl, MockPlatform::getBytesFromFile(scene_file));

    scene = std::make_shared<Scene>(*platform, sceneUrl);
    Importer importer(scene);
    try {
        scene->config() = importer.applySceneImports(*platform.get());
    }
    catch (const YAML::ParserException& e) {
        LOGE("Parsing scene config '%s'", e.what());
        exit(-1);
    }
    if (!scene->config()) {
        LOGE("Invalid scene file '%s'", scene_file);
        exit(-1);
    }
    SceneLoader::applyConfig(*platform.get(), scene);
    scene->fontContext()->loadFonts();

    for (auto& s : scene->tileSources()) {
        source = s;
        if (source->generateGeometry()) { break; }
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
    std::shared_ptr<Tile> result;
    void SetUp(const ::benchmark::State& state) override {
        globalSetup();
        tileBuilder = std::make_unique<TileBuilder>(scene, new StyleContext());
    }
    void TearDown(const ::benchmark::State& state) override {
        result.reset();
    }

    __attribute__ ((noinline)) void run() {
        result = tileBuilder->build({0,0,10,10}, *tileData, *source);
    }
};

RUN(TileBuilderFixture, TileBuilderBench);



BENCHMARK_MAIN();
