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
    // Tile ID
    const TileID tileId;

    // Save shared reference to Datasource while building tile
    std::shared_ptr<DataSource> source;

    // Raw tile data that will be processed by DataSource.
    std::shared_ptr<std::vector<char>> rawTileData;

    // Tile result, set when tile was  sucessfully created
    std::shared_ptr<Tile> tile;

    bool loaded = false;
    bool canceled = false;
    bool visible = true;

    std::atomic<double> priority;

    TileTask(TileID& _tileId, std::shared_ptr<DataSource> _source) :
        tileId(_tileId),
        source(_source),
        sourceGeneration(_source->generation()) {}

    virtual std::shared_ptr<TileData> process(MapProjection& _projection);

    virtual ~TileTask() {}

    TileTask& operator=(const TileTask& _other) = delete;

    double getPriority() const {
        return priority.load();
    }

    void setPriority(double _priority) {
        priority.store(_priority);
    }

    bool isCanceled() const { return canceled; }

    bool isReady() const { return bool(tile); }

    void cancel() {
        canceled = true;
        tile.reset();
    }

    const int64_t sourceGeneration;

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
