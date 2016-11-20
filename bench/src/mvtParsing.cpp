#include "tangram.h"
#include "log.h"
#include "data/tileSource.h"
#include "data/mvtSource.h"
#include "data/tileData.h"
#include "util/mapProjection.h"
#include "tile/tile.h"
#include "tile/tileTask.h"

#include <vector>
#include <iostream>
#include <fstream>

#include "benchmark/benchmark_api.h"
#include "benchmark/benchmark.h"

using namespace Tangram;

struct TestTileDataSink : Tangram::TileDataSink {
    bool beginLayer(const std::string& _layer) override {
        benchmark::DoNotOptimize(&_layer);

        return true;
    }
    bool matchFeature(const Feature& _feature) override {
        benchmark::DoNotOptimize(&_feature);

        return true;
    }

    void addFeature(const Feature& _feature) override {
        benchmark::DoNotOptimize(&_feature);
    }

};
struct TestContext {

    MercatorProjection projection;

    std::shared_ptr<TileSource> source;
    std::shared_ptr<std::vector<char>> rawTileData;
    std::shared_ptr<TileData> tileData;

    TestTileDataSink sink;

    void loadTile(const char* path){
        std::ifstream resource(path, std::ifstream::ate | std::ifstream::binary);
        if(!resource.is_open()) {
            LOGE("Failed to read file at path: %s", path);
            return;
        }

        size_t _size = resource.tellg();
        resource.seekg(std::ifstream::beg);

        rawTileData = std::make_shared<std::vector<char>>();
        rawTileData->resize(_size);

        resource.read(&(*rawTileData)[0], _size);
        resource.close();
    }
};

class MVTParsingFixture : public benchmark::Fixture {
public:
    TestContext ctx;

    const char* tileFile = "tile.mvt";

    void SetUp() override {
        LOG("SETUP");
        ctx.source = std::make_shared<MVTSource>("", nullptr, 20);
        ctx.loadTile(tileFile);
    }
    void TearDown() override {
        LOG("TEARDOWN");
    }
};

BENCHMARK_DEFINE_F(MVTParsingFixture, BuildTest)(benchmark::State& st) {

    while (st.KeepRunning()) {
        TileID tileId{0,0,10,10,0};
        Tile tile(tileId, ctx.projection);

        auto task = std::make_shared<BinaryTileTask>(tileId, ctx.source, -1);
        task->rawTileData = ctx.rawTileData;

        bool ok = ctx.source->process(*task, ctx.projection, ctx.sink);

        benchmark::DoNotOptimize(ok);
    }
}

BENCHMARK_REGISTER_F(MVTParsingFixture, BuildTest);



BENCHMARK_MAIN();
