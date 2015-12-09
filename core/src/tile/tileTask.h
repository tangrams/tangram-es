#pragma once

#include "data/dataSource.h"
#include "tile/tileID.h"

#include <memory>
#include <vector>
#include <functional>
#include <atomic>

namespace Tangram {

class TileManager;
class DataSource;
class Tile;
struct TileData;


class TileTask {

public:

    TileTask(TileID& _tileId, std::shared_ptr<DataSource> _source) :
        m_tileId(_tileId),
        m_source(_source),
        m_sourceGeneration(_source->generation()) {}

    // No copies
    TileTask(const TileTask& _other) = delete;
    TileTask& operator=(const TileTask& _other) = delete;

    virtual ~TileTask() {}

    virtual std::shared_ptr<TileData> process(MapProjection& _projection) = 0;

    virtual bool hasData() const = 0;

    void cancel() {  m_canceled = true; }

    bool isCanceled() const { return m_canceled; }

    double getPriority() const {
        return m_priority.load();
    }

    void setPriority(double _priority) {
        m_priority.store(_priority);
    }

    void setTile(std::shared_ptr<Tile>&& _tile) {
        m_tile = std::move(_tile);
    }

    std::shared_ptr<Tile>& tile() { return m_tile; }

    bool isReady() const { return bool(m_tile); }

    DataSource& source() { return *m_source; }
    int64_t sourceGeneration() const { return m_sourceGeneration; }

    TileID tileId() const { return m_tileId; }

protected:

    const TileID m_tileId;

    // Save shared reference to Datasource while building tile
    std::shared_ptr<DataSource> m_source;

    const int64_t m_sourceGeneration;

    // Tile result, set when tile was  sucessfully created
    std::shared_ptr<Tile> m_tile;

    bool m_canceled = false;

    std::atomic<double> m_priority;
    // bool visible = true;

};

class DownloadTileTask : public TileTask {
public:
    DownloadTileTask(TileID& _tileId, std::shared_ptr<DataSource> _source)
        : TileTask(_tileId, _source) {}

    virtual std::shared_ptr<TileData> process(MapProjection& _projection) override {
        return m_source->parse(*this, _projection);
    }

    virtual bool hasData() const override {
        return rawTileData && !rawTileData->empty();
    }
    // Raw tile data that will be processed by DataSource.
    std::shared_ptr<std::vector<char>> rawTileData;
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
