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
    const TileID tileId;

    // NB: Save shared reference to Datasource while building tile
    std::shared_ptr<DataSource> source;

    // Raw tile data that will be processed by DataSource.
    std::shared_ptr<std::vector<char>> rawTileData;

    //
    std::shared_ptr<Tile> tile;

    bool loaded = false;
    bool canceled = false;

    virtual std::shared_ptr<TileData> process();
    bool visible = true;

    std::atomic<double> priority;

    TileTask(TileID& _tileId, std::shared_ptr<DataSource> _source) :
        tileId(_tileId),
        source(_source),
        sourceGeneration(_source->generation()) {}

    TileTask& operator=(const TileTask& _other) = delete;

    double getPriority() const {
        return priority.load();
    }

    void setPriority(double _priority) {
        priority.store(_priority);
    }

    bool isCanceled() const { return canceled; }

    void cancel() { canceled = true; }

    const int64_t sourceGeneration;

};

struct TileTaskCb {
    std::function<void(std::shared_ptr<TileTask>&&)> func;
};

}
