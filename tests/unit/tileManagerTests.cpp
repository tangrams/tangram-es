#include "catch.hpp"

#include "data/tileSource.h"
#include "mockPlatform.h"
#include "tile/tileManager.h"
#include "tile/tileWorker.h"
#include "util/mapProjection.h"
#include "util/fastmap.h"
#include "view/view.h"

#include <deque>

using namespace Tangram;

ViewState viewState { true, glm::vec2(0), 1, 0, 1.f, glm::vec2(0), 256.f };

struct TestTileWorker : TileTaskQueue {
    int processedCount = 0;
    bool pendingTiles = false;

    std::deque<std::shared_ptr<TileTask>> tasks;

    void enqueue(std::shared_ptr<TileTask> task) override{
        tasks.push_back(std::move(task));
    }

    bool checkProcessedTiles() {
        if (pendingTiles) {
            pendingTiles = false;
            return true;
        }
        return false;
    }

    void processTask() {
        while (!tasks.empty()) {
            auto task = tasks.front();
            tasks.pop_front();
            if (task->isCanceled()) {
                continue;
            }

            task->setTile(std::make_unique<Tile>(task->tileId(),
                                                 task->source()->id(),
                                                 task->source()->generation()));

            pendingTiles = true;
            processedCount++;
            break;
        }
    }
    void processTask(int position) {

        auto task = tasks[position];
        tasks.erase(tasks.begin() + position);

        task->setTile(std::make_unique<Tile>(task->tileId(),
                                             task->source()->id(),
                                             task->source()->generation()));

        pendingTiles = true;
        processedCount++;
    }

    void dropTask() {
        if (!tasks.empty()) {
            auto task = tasks.front();
            tasks.pop_front();
            task->cancel();
        }
    }
};

struct TestTileSource : TileSource {
    class Task : public TileTask {
    public:
        bool gotData = false;

        Task(TileID& _tileId, std::shared_ptr<TileSource> _source)
            : TileTask(_tileId, _source) {}

        bool hasData() const override { return gotData; }
    };

    int tileTaskCount = 0;

    TestTileSource() : TileSource("test", nullptr) {
        m_generateGeometry = true;
    }

    void loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override {
        tileTaskCount++;
        static_cast<Task*>(_task.get())->gotData = true;
        _task->startedLoading();
        _cb.func(std::move(_task));
    }

    void cancelLoadingTile(TileTask& _tile) override {}

    std::shared_ptr<TileData> parse(const TileTask& _task) const override {
        return nullptr;
    };

    void clearData() override {}

    std::shared_ptr<TileTask> createTask(TileID _tileId) override {
        return std::make_shared<Task>(_tileId, shared_from_this());
    }
};

class TestTileManager : public TileManager {
public:
    using Base = TileManager;
    using Base::Base;

    void updateTiles(const ViewState& _view, std::set<TileID> _visibleTiles) {
        // Mimic TileManager::updateTileSets(View& _view)
        m_tiles.clear();
        m_tilesInProgress = 0;
        m_tileSetChanged = false;

        TileSet& tileSet = m_tileSets[0];

        tileSet.visibleTiles = _visibleTiles;

        TileManager::updateTileSet(tileSet, _view);

        loadTiles();

        // Make m_tiles an unique list of tiles for rendering sorted from
        // high to low zoom-levels.
        std::sort(m_tiles.begin(), m_tiles.end(), [](auto& a, auto& b){
                return a->getID() < b->getID(); });

        // Remove duplicates: Proxy tiles could have been added more than once
        m_tiles.erase(std::unique(m_tiles.begin(), m_tiles.end()), m_tiles.end());

    }
};

TEST_CASE( "Use proxy Tile - Dont remove proxy if it is now visible", "[TileManager][updateTileSets]" ) {
    TestTileWorker worker;
    MockPlatform platform;
    TestTileManager tileManager(platform, worker);

    auto source = std::make_shared<TestTileSource>();
    std::vector<std::shared_ptr<TileSource>> sources = { source };
    tileManager.setTileSources(sources);

    /// Start loading tile 0/0/0
    std::set<TileID> visibleTiles_1 = {TileID{0,0,0}};
    tileManager.updateTiles(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 1);
    REQUIRE(worker.processedCount == 0);

    /// Start loading tile 0/0/1 - uses 0/0/0 as proxy
    std::set<TileID> visibleTiles_2 = {TileID{0,0,1}};

    tileManager.updateTiles(viewState, visibleTiles_2);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 0);

    /// Process tile task 0/0/1
    worker.processTask(1);

    /// Go back to tile 0/0/0 - uses 0/0/1 as proxy
    tileManager.updateTiles(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == true);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,1));

    // Process tile task 0/0/0
    worker.processTask(0);
    tileManager.updateTiles(viewState, visibleTiles_1);
    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == false);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,0));
}

TEST_CASE( "Mock TileWorker Initialization", "[TileManager][Constructor]" ) {

    TestTileWorker worker;
    MockPlatform platform;
    TileManager tileManager(platform, worker);
}

TEST_CASE( "Real TileWorker Initialization", "[TileManager][Constructor]" ) {
    MockPlatform platform;
    TileWorker worker(platform, 1);
    TileManager tileManager(platform, worker);
}

TEST_CASE( "Load visible Tile", "[TileManager][updateTileSets]" ) {
    TestTileWorker worker;
    MockPlatform platform;
    TestTileManager tileManager(platform, worker);

    auto source = std::make_shared<TestTileSource>();
    std::vector<std::shared_ptr<TileSource>> sources = { source };
    tileManager.setTileSources(sources);

    std::set<TileID> visibleTiles = {TileID{0,0,0}};
    tileManager.updateTiles(viewState, visibleTiles);
    worker.processTask();

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 1);
    REQUIRE(worker.processedCount == 1);

    tileManager.updateTiles(viewState, visibleTiles);

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(source->tileTaskCount == 1);
    REQUIRE(worker.processedCount == 1);

}


TEST_CASE( "Use proxy Tile", "[TileManager][updateTileSets]" ) {
    TestTileWorker worker;
    MockPlatform platform;
    TestTileManager tileManager(platform, worker);

    auto source = std::make_shared<TestTileSource>();
    std::vector<std::shared_ptr<TileSource>> sources = { source };
    tileManager.setTileSources(sources);

    std::set<TileID> visibleTiles = {TileID{0,0,0}};
    tileManager.updateTiles(viewState, visibleTiles);
    worker.processTask();

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 1);
    REQUIRE(worker.processedCount == 1);

    std::set<TileID> visibleTiles2 = {TileID{0,0,1}};
    tileManager.updateTiles(viewState, visibleTiles2);
    worker.processTask();

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == true);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,0));
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 2);

    tileManager.updateTiles(viewState, visibleTiles2);
    worker.processTask();

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == false);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,1));
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 2);

}


TEST_CASE( "Use proxy Tile - circular proxies", "[TileManager][updateTileSets]" ) {
    TestTileWorker worker;
    MockPlatform platform;
    TestTileManager tileManager(platform, worker);

    auto source = std::make_shared<TestTileSource>();
    std::vector<std::shared_ptr<TileSource>> sources = { source };
    tileManager.setTileSources(sources);

    /// Start loading tile 0/0/0
    std::set<TileID> visibleTiles_1 = {TileID{0,0,0}};
    tileManager.updateTiles(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 1);
    REQUIRE(worker.processedCount == 0);

    /// Start loading tile 0/0/1 - add 0/0/0 as proxy
    std::set<TileID> visibleTiles_2 = {TileID{0,0,1}};
    tileManager.updateTiles(viewState, visibleTiles_2);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 0);

    /// Go back to tile 0/0/0
    /// NB: does not add 0/0/1 as proxy, since no newTiles were loaded
    tileManager.updateTiles(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 0);

    REQUIRE(worker.tasks.size() == 2);
    // tile 0/0/0 still loading
    REQUIRE(worker.tasks[0]->isCanceled() == false);
    // tile 0/0/1 canceled
    REQUIRE(worker.tasks[1]->isCanceled() == true);

    worker.processTask();
    tileManager.updateTiles(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == false);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,0));

}
