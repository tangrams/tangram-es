#include "catch.hpp"

#include "data/dataSource.h"
#include "tile/tileManager.h"
#include "tile/tileWorker.h"
#include "util/mapProjection.h"

#include <deque>

using namespace Tangram;

MercatorProjection s_projection;

struct TestTileWorker : TileTaskQueue {
    int processedCount = 0;

    std::deque<std::shared_ptr<TileTask>> tasks;

    virtual void enqueue(std::shared_ptr<TileTask>&& task) {
        tasks.push_back(std::move(task));
    }

    virtual bool checkProcessedTiles() { return true; }

    void processTask() {
        while (!tasks.empty()) {
            auto task = tasks.front();
            tasks.pop_front();
            if (task->isCanceled()) {
                continue;
            }
            task->setTile(std::make_shared<Tile>(task->tileId(), s_projection, &task->source()));

            processedCount++;
            break;
        }
    }
    void dropTask() {
        if (!tasks.empty()) {
            auto task = tasks.front();
            tasks.pop_front();
            task->cancel();
        }
    }
};

struct TestDataSource : DataSource {
    class Task : public TileTask {
    public:
        bool gotData = false;

        Task(TileID& _tileId, std::shared_ptr<DataSource> _source)
            : TileTask(_tileId, _source) {}

        virtual bool hasData() const override { return gotData; }
    };

    int tileTaskCount = 0;

    TestDataSource() : DataSource("", "") {}
    virtual bool loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) {
        tileTaskCount++;
        static_cast<Task*>(_task.get())->gotData = true;
        _cb.func(std::move(_task));
        return true;
    }

    virtual void cancelLoadingTile(const TileID& _tile) {}

    virtual std::shared_ptr<TileData> parse(const TileTask& _task,
                                            const MapProjection& _projection) const {
        return nullptr;
    };

    virtual void clearData() {}

    virtual std::shared_ptr<TileTask> createTask(TileID _tileId) {
        return std::make_shared<Task>(_tileId, shared_from_this());
    }
};

TEST_CASE( "Use proxy Tile - Dont remove proxy if it is now visible", "[TileManager][updateTileSets]" ) {
    TestTileWorker worker;
    TileManager tileManager(worker);
    ViewState viewState { s_projection, true, glm::vec2(0), 1 };

    auto source = std::make_shared<TestDataSource>();
    std::vector<std::shared_ptr<DataSource>> sources = { source };
    tileManager.setDataSources(sources);

    /// Start loading tile 0/0/0
    std::set<TileID> visibleTiles_1 = { TileID{0,0,0} };
    tileManager.updateTileSets(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 1);
    REQUIRE(worker.processedCount == 0);

    /// Start loading tile 0/0/1 - uses 0/0/0 as proxy
    std::set<TileID> visibleTiles_2 = { TileID{0,0,1} };
    tileManager.updateTileSets(viewState, visibleTiles_2);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 0);

    /// Drop tile task 0/0/0
    worker.dropTask();

    /// Process tile task 0/0/1
    worker.processTask();

    /// Go back to tile 0/0/0 - uses 0/0/1 as proxy
    tileManager.updateTileSets(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(source->tileTaskCount == 3);
    REQUIRE(worker.processedCount == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == true);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,1));

    /// Process tile task 0/0/0
    worker.processTask();
    tileManager.updateTileSets(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == false);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,0));
}

TEST_CASE( "Mock TileWorker Initialization", "[TileManager][Constructor]" ) {

    TestTileWorker worker;
    TileManager tileManager(worker);
}

TEST_CASE( "Real TileWorker Initialization", "[TileManager][Constructor]" ) {
    TileWorker worker(1);
    TileManager tileManager(worker);
}

TEST_CASE( "Load visible Tile", "[TileManager][updateTileSets]" ) {
    TestTileWorker worker;
    TileManager tileManager(worker);
    ViewState viewState { s_projection, true, glm::vec2(0), 1 };

    auto source = std::make_shared<TestDataSource>();
    std::vector<std::shared_ptr<DataSource>> sources = { source };
    tileManager.setDataSources(sources);

    std::set<TileID> visibleTiles = { TileID{0,0,0} };
    tileManager.updateTileSets(viewState, visibleTiles);
    worker.processTask();

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 1);
    REQUIRE(worker.processedCount == 1);

    tileManager.updateTileSets(viewState, visibleTiles);

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(source->tileTaskCount == 1);
    REQUIRE(worker.processedCount == 1);

}


TEST_CASE( "Use proxy Tile", "[TileManager][updateTileSets]" ) {
    TestTileWorker worker;
    TileManager tileManager(worker);
    ViewState viewState { s_projection, true, glm::vec2(0), 1 };

    auto source = std::make_shared<TestDataSource>();
    std::vector<std::shared_ptr<DataSource>> sources = { source };
    tileManager.setDataSources(sources);

    std::set<TileID> visibleTiles = { TileID{0,0,0} };
    tileManager.updateTileSets(viewState, visibleTiles);
    worker.processTask();

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 1);
    REQUIRE(worker.processedCount == 1);

    std::set<TileID> visibleTiles2 = { TileID{0,0,1} };
    tileManager.updateTileSets(viewState, visibleTiles2);
    worker.processTask();

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == true);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,0));
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 2);

    tileManager.updateTileSets(viewState, visibleTiles2);
    worker.processTask();

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == false);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,1));
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 2);

}


TEST_CASE( "Use proxy Tile - circular proxies", "[TileManager][updateTileSets]" ) {
    TestTileWorker worker;
    TileManager tileManager(worker);
    ViewState viewState { s_projection, true, glm::vec2(0), 1 };

    auto source = std::make_shared<TestDataSource>();
    std::vector<std::shared_ptr<DataSource>> sources = { source };
    tileManager.setDataSources(sources);

    /// Start loading tile 0/0/0
    std::set<TileID> visibleTiles_1 = { TileID{0,0,0} };
    tileManager.updateTileSets(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 1);
    REQUIRE(worker.processedCount == 0);

    /// Start loading tile 0/0/1 - add 0/0/0 as proxy
    std::set<TileID> visibleTiles_2 = { TileID{0,0,1} };
    tileManager.updateTileSets(viewState, visibleTiles_2);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 0);

    /// Go back to tile 0/0/0 - add 0/0/1 as proxy
    tileManager.updateTileSets(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 0);

    REQUIRE(worker.tasks.size() == 2);
    REQUIRE(worker.tasks[0]->isCanceled() == false);
    REQUIRE(worker.tasks[1]->isCanceled() == true);

    worker.processTask();
    tileManager.updateTileSets(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == false);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,0));

}
