#pragma once

#include "mapTile.h"
#include "tileData.h"

#include <memory>

class TileManager;
class DataSource;

class TileTask {

public:
    std::shared_ptr<MapTile> tile;
    /*const*/ DataSource* source;

    // Only one of either parsedTileData or rawTileData will be non-empty for a given task.
    // If parsedTileData is non-empty, then the data for this tile was previously fetched
    // and parsed. Otherwise rawTileData will be non-empty, indicating that the data needs
    // to be parsed using the given DataSource.
    std::shared_ptr<TileData> parsedTileData;
    std::vector<char> rawTileData;

    TileTask(std::shared_ptr<MapTile> _tile, DataSource* _source) :
        tile(_tile),
        source(_source) {
    }

    TileTask& operator=(const TileTask& _other) = delete;
};

typedef std::function<void(std::shared_ptr<TileTask>&&)> TileTaskCb;
