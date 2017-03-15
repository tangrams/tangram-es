#include "catch.hpp"

#include "data/tileSource.h"
#include "tile/tileManager.h"
#include "tile/tileWorker.h"
#include "util/mapProjection.h"
#include "util/fastmap.h"
#include "view/view.h"
#include "platform_mock.h"

#include <deque>

using namespace Tangram;

MercatorProjection s_projection;
ViewState viewState { &s_projection, true, glm::vec2(0), 1, 0, 1.f, glm::vec2(0), 256.f };

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

            task->tile() = std::make_shared<Tile>(task->tileId(), s_projection, &task->source());

            pendingTiles = true;
            processedCount++;
            break;
        }
    }
    void processTask(int position) {

        auto task = tasks[position];
        tasks.erase(tasks.begin() + position);

        task->tile() = std::make_shared<Tile>(task->tileId(), s_projection, &task->source());

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

        Task(TileID& _tileId, std::shared_ptr<TileSource> _source, bool _subTask)
            : TileTask(_tileId, _source, _subTask) {}

        bool hasData() const override { return gotData; }
    };

    virtual const char* mimeType() override { return ""; };

    int tileTaskCount = 0;

    TestTileSource() : TileSource("", nullptr) {
        m_generateGeometry = true;
    }

    void loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) override {
        tileTaskCount++;
        static_cast<Task*>(_task.get())->gotData = true;
        _task->startedLoading();
        _cb.func(std::move(_task));
    }

    void cancelLoadingTile(const TileID& _tile) override {}

    std::shared_ptr<TileData> parse(const TileTask& _task,
                                    const MapProjection& _projection) const override{
        return nullptr;
    };

    void clearData() override {}

    std::shared_ptr<TileTask> createTask(TileID _tileId, int _subTask) override {
        return std::make_shared<Task>(_tileId, shared_from_this(), _subTask);
    }
};

TEST_CASE( "Use proxy Tile - Dont remove proxy if it is now visible", "[TileManager][updateTileSets]" ) {
    TestTileWorker worker;
    TileManager tileManager(std::make_shared<MockPlatform>(), worker);

    auto source = std::make_shared<TestTileSource>();
    std::vector<std::shared_ptr<TileSource>> sources = { source };
    tileManager.setTileSources(sources);

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

    /// Process tile task 0/0/1
    worker.processTask(1);

    /// Go back to tile 0/0/0 - uses 0/0/1 as proxy
    tileManager.updateTileSets(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == true);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,1));

    // Process tile task 0/0/0
    worker.processTask(0);
    tileManager.updateTileSets(viewState, visibleTiles_1);
    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == false);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,0));
}

TEST_CASE( "Mock TileWorker Initialization", "[TileManager][Constructor]" ) {

    TestTileWorker worker;
    TileManager tileManager(std::shared_ptr<MockPlatform>(), worker);
}

TEST_CASE( "Real TileWorker Initialization", "[TileManager][Constructor]" ) {
    auto platform = std::make_shared<MockPlatform>();
    TileWorker worker(platform, 1);
    TileManager tileManager(platform, worker);
}

TEST_CASE( "Load visible Tile", "[TileManager][updateTileSets]" ) {
    TestTileWorker worker;
    TileManager tileManager(std::make_shared<MockPlatform>(), worker);

    auto source = std::make_shared<TestTileSource>();
    std::vector<std::shared_ptr<TileSource>> sources = { source };
    tileManager.setTileSources(sources);

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
    TileManager tileManager(std::make_shared<MockPlatform>(), worker);

    auto source = std::make_shared<TestTileSource>();
    std::vector<std::shared_ptr<TileSource>> sources = { source };
    tileManager.setTileSources(sources);

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
    TileManager tileManager(std::make_shared<MockPlatform>(), worker);

    auto source = std::make_shared<TestTileSource>();
    std::vector<std::shared_ptr<TileSource>> sources = { source };
    tileManager.setTileSources(sources);

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

    /// Go back to tile 0/0/0
    /// NB: does not add 0/0/1 as proxy, since no newTiles were loaded
    tileManager.updateTileSets(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 0);
    REQUIRE(source->tileTaskCount == 2);
    REQUIRE(worker.processedCount == 0);

    REQUIRE(worker.tasks.size() == 2);
    // tile 0/0/0 still loading
    REQUIRE(worker.tasks[0]->isCanceled() == false);
    // tile 0/0/1 canceled
    REQUIRE(worker.tasks[1]->isCanceled() == true);

    worker.processTask();
    tileManager.updateTileSets(viewState, visibleTiles_1);

    REQUIRE(tileManager.getVisibleTiles().size() == 1);
    REQUIRE(tileManager.getVisibleTiles()[0]->isProxy() == false);
    REQUIRE(tileManager.getVisibleTiles()[0]->getID() == TileID(0,0,0));

}
