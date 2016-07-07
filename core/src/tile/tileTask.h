#pragma once

#include "tile/tileID.h"

#include <memory>
#include <vector>
#include <functional>
#include <atomic>

namespace Tangram {

class TileManager;
class TileBuilder;
class DataSource;
class Tile;
class MapProjection;
struct TileData;


class TileTask {

public:

    TileTask(TileID& _tileId, std::shared_ptr<DataSource> _source, int _subTask);

    // No copies
    TileTask(const TileTask& _other) = delete;
    TileTask& operator=(const TileTask& _other) = delete;

    virtual ~TileTask() {}

    virtual bool hasData() const { return true; }

    virtual bool isReady() const { return bool(m_tile); }

    std::shared_ptr<Tile>& tile() { return m_tile; }

    DataSource& source() { return *m_source; }
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

protected:

    const TileID m_tileId;

    const int m_subTaskId;

    // Save shared reference to Datasource while building tile
    std::shared_ptr<DataSource> m_source;

    // Vector of tasks to download raster samplers
    std::vector<std::shared_ptr<TileTask>> m_subTasks;

    const int64_t m_sourceGeneration;

    // Tile result, set when tile was  sucessfully created
    std::shared_ptr<Tile> m_tile;

    bool m_canceled = false;

    std::atomic<float> m_priority;
    bool m_proxyState = false;
};

class DownloadTileTask : public TileTask {
public:
    DownloadTileTask(TileID& _tileId, std::shared_ptr<DataSource> _source, int _subTask)
        : TileTask(_tileId, _source, _subTask) {}

    virtual bool hasData() const override {
        return rawTileData && !rawTileData->empty();
    }
    // Raw tile data that will be processed by DataSource.
    std::shared_ptr<std::vector<char>> rawTileData;
};

struct TileTaskQueue {
    virtual void enqueue(std::shared_ptr<TileTask>&& task) = 0;
};

struct TileTaskCb {
    std::function<void(std::shared_ptr<TileTask>&&)> func;
};

}
