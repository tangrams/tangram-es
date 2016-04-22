#pragma once

#include "tile/tileID.h"

#include <memory>
#include <vector>
#include <functional>
#include <atomic>

namespace Tangram {

class TileManager;
class DataSource;
class Tile;
class MapProjection;
struct TileData;


class TileTask {

public:

    TileTask(TileID& _tileId, std::shared_ptr<DataSource> _source);

    // No copies
    TileTask(const TileTask& _other) = delete;
    TileTask& operator=(const TileTask& _other) = delete;

    virtual ~TileTask() {}

    virtual bool hasData() const { return true; }

    void setTile(std::shared_ptr<Tile>&& _tile) {
        m_tile = std::move(_tile);
    }

    std::shared_ptr<Tile>& tile() { return m_tile; }

    bool isReady() const { return bool(m_tile); }

    DataSource& source() { return *m_source; }
    int64_t sourceGeneration() const { return m_sourceGeneration; }

    TileID tileId() const { return m_tileId; }

    void cancel();
    void doneBuilding();

    bool isCanceled() const { return m_canceled; }
    bool isBuilt() const { return m_doneBuilding; }

    double getPriority() const {
        return m_priority.load();
    }

    void setPriority(double _priority) {
        m_priority.store(_priority);
    }

    bool isProxy() const { return m_proxyState; }

    void setProxyState(bool isProxy) { m_proxyState = isProxy; }
    auto& rasterTasks() { return m_rasterTasks; }

protected:

    const TileID m_tileId;

    // Save shared reference to Datasource while building tile
    std::shared_ptr<DataSource> m_source;

    // Vector of tasks to download raster samplers
    std::vector<std::shared_ptr<TileTask>> m_rasterTasks;

    const int64_t m_sourceGeneration;

    // Tile result, set when tile was  sucessfully created
    std::shared_ptr<Tile> m_tile;

    bool m_canceled = false;
    bool m_doneBuilding = false;

    std::atomic<double> m_priority;
    bool m_proxyState = false;
};

class DownloadTileTask : public TileTask {
public:
    DownloadTileTask(TileID& _tileId, std::shared_ptr<DataSource> _source)
        : TileTask(_tileId, _source) {}

    virtual bool hasData() const override {
        return rasterReady || (rawTileData && !rawTileData->empty());
    }
    // Raw tile data that will be processed by DataSource.
    std::shared_ptr<std::vector<char>> rawTileData;

    // OkHttp returns empty rawData on a bad url fetch. This make sures the rasterTasks are not
    // blocking the worker threads for eternity.
    bool rasterReady = false;
};

struct TileTaskQueue {
    virtual void enqueue(std::shared_ptr<TileTask>&& task) = 0;

    // Check processed-tiles flag. Resets flag on each call..
    // TODO better name checkAndResetProcessedTilesFlag?
    virtual bool checkProcessedTiles() = 0;

};

struct TileTaskCb {
    std::function<void(std::shared_ptr<TileTask>&&)> func;
};

}
