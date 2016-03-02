#include "tangram.h"
#include "platform.h"
#include "data/dataSource.h"
#include "data/mvtSource.h"
#include "util/mapProjection.h"
#include "tile/tile.h"
#include "tile/tileTask.h"

#include <vector>
#include <iostream>
#include <fstream>

#include "benchmark/benchmark_api.h"
#include "benchmark/benchmark.h"

using namespace Tangram;

struct TestContext {

    MercatorProjection projection;

    std::shared_ptr<DataSource> source;
    std::shared_ptr<std::vector<char>> rawTileData;
    std::shared_ptr<TileData> tileData;

    void loadTile(const char* path){
        std::ifstream resource(path, std::ifstream::ate | std::ifstream::binary);
        if(!resource.is_open()) {
            LOGE("Failed to read file at path: %s", path.c_str());
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
        ctx.source = std::make_shared<MVTSource>("","",20);
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
        
        auto task = std::make_shared<DownloadTileTask>(tileId, ctx.source);
        task->rawTileData = ctx.rawTileData;

        auto tileData = ctx.source->parse(*task, ctx.projection);

        benchmark::DoNotOptimize(tileData);
    }
}

BENCHMARK_REGISTER_F(MVTParsingFixture, BuildTest);



BENCHMARK_MAIN();
