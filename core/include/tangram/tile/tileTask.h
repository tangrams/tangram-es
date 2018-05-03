#pragma once

#include "tile/tileID.h"

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace Tangram {

class TileManager;
class TileBuilder;
class TileSource;
class Tile;
class MapProjection;
struct TileData;


class TileTask {

public:

    TileTask(TileID& _tileId, std::shared_ptr<TileSource> _source, int _subTask);

    // No copies
    TileTask(const TileTask& _other) = delete;
    TileTask& operator=(const TileTask& _other) = delete;

    virtual ~TileTask();

    virtual bool hasData() const { return true; }

    virtual bool isReady() const {
        if (needsLoading()) { return false; }

        return bool(m_ready);
    }

    Tile* tile() { return m_tile.get(); }

    std::unique_ptr<Tile> getTile();
    void setTile(std::unique_ptr<Tile>&& _tile);

    TileSource& source() { return *m_source; }
    int64_t sourceGeneration() const { return m_sourceGeneration; }

    TileID tileId() const { return m_tileId; }

    void cancel() { m_canceled = true; }
    bool isCanceled() const { return m_canceled; }

    double getPriority() const {
        return m_priority.load();
    }

    void setPriority(double _priority) {
        m_priority.store(_priority);
    }

    void setProxyState(bool isProxy) { m_proxyState = isProxy; }
    bool isProxy() const { return m_proxyState; }

    auto& subTasks() { return m_subTasks; }
    int subTaskId() const { return m_subTaskId; }
    bool isSubTask() const { return m_subTaskId >= 0; }

    // running on worker thread
    virtual void process(TileBuilder& _tileBuilder);

    // running on main thread when the tile is added to
    virtual void complete();

    // onDone for sub-tasks
    virtual void complete(TileTask& _mainTask) {}

    int rawSource = 0;

    bool needsLoading() const { return m_needsLoading; }

    // Set whether DataSource should (re)try loading data
    void setNeedsLoading(bool _needsLoading) {
         m_needsLoading = _needsLoading;
    }

    void startedLoading() { m_needsLoading = false; }

protected:

    const TileID m_tileId;

    const int m_subTaskId;

    // Save shared reference to Datasource while building tile
    std::shared_ptr<TileSource> m_source;

    // Vector of tasks to download raster samplers
    std::vector<std::shared_ptr<TileTask>> m_subTasks;

    const int64_t m_sourceGeneration;

    // Tile result, set when tile was  sucessfully created
    std::unique_ptr<Tile> m_tile;

    std::atomic<bool> m_ready;
    std::atomic<bool> m_canceled;
    std::atomic<bool> m_needsLoading;

    std::atomic<float> m_priority;
    std::atomic<bool> m_proxyState;
};

class BinaryTileTask : public TileTask {
public:
    BinaryTileTask(TileID& _tileId, std::shared_ptr<TileSource> _source, int _subTask)
        : TileTask(_tileId, _source, _subTask) {}

    virtual bool hasData() const override {
        return rawTileData && !rawTileData->empty();
    }
    // Raw tile data that will be processed by TileSource.
    std::shared_ptr<std::vector<char>> rawTileData;

    bool dataFromCache = false;
};

struct TileTaskQueue {
    virtual void enqueue(std::shared_ptr<TileTask> task) = 0;
};

struct TileTaskCb {
    std::function<void(std::shared_ptr<TileTask>)> func;
};

}
