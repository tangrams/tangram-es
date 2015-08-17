#pragma once

#include "tile.h"
#include "data/tileData.h"

#include <memory>

namespace Tangram {

class TileManager;
class DataSource;

class TileTask {

public:
    std::shared_ptr<Tile> tile;
    /*const*/ DataSource* source;

    // Raw tile data that will be processed by DataSource.
    std::shared_ptr<std::vector<char>> rawTileData;

    TileTask(std::shared_ptr<Tile> _tile, DataSource* _source) :
        tile(_tile),
        source(_source) {
    }
    virtual std::shared_ptr<TileData> process();

    TileTask& operator=(const TileTask& _other) = delete;
};

typedef std::function<void(std::shared_ptr<TileTask>&&)> TileTaskCb;

}
