#include "benchmark/benchmark.h"

#include "data/tileSource.h"
#include "log.h"
#include "map.h"
#include "mockPlatform.h"
#include "tile/tile.h"
#include "tile/tileTask.h"

using namespace Tangram;

const char tile_file[] = "res/tile.mvt";

struct TileSourceFixture : public benchmark::Fixture {
    std::shared_ptr<TileSource> source;
    std::shared_ptr<TileTask> tileTask;
    std::shared_ptr<TileData> tileData;

    void SetUp(const ::benchmark::State& state) override {
        Tile tile({0,0,10,10});

        source = std::make_shared<TileSource>("test", nullptr);
        source->setFormat(TileSource::Format::Mvt);

        tileTask = source->createTask(tile.getID());

        auto rawTileData = MockPlatform::getBytesFromFile(tile_file);
        auto& t = dynamic_cast<BinaryTileTask&>(*tileTask);
        t.rawTileData = std::make_shared<std::vector<char>>(rawTileData);
    }
    void TearDown(const ::benchmark::State& state) override {
    }
};
BENCHMARK_DEFINE_F(TileSourceFixture, TileSourceBench)(benchmark::State& st) {
    while (st.KeepRunning()) {

        tileData = source->parse(*tileTask);

        if (!tileData) {
            LOGE("Invalid tile file '%s'", tile_file);
            exit(-1);
        }
    }
}
BENCHMARK_REGISTER_F(TileSourceFixture, TileSourceBench);


BENCHMARK_MAIN();
