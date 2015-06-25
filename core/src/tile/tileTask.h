#pragma once

#include "mapTile.h"
#include "tileData.h"

class TileManager;
class DataSource;


class TileTaskData {

public:
    TileID tileID;
    /*const*/ DataSource* source;

    // Only one of either parsedTileData or rawTileData will be non-empty for a given task.
    // If parsedTileData is non-empty, then the data for this tile was previously fetched
    // and parsed. Otherwise rawTileData will be non-empty, indicating that the data needs
    // to be parsed using the given DataSource.
    std::shared_ptr<TileData> parsedTileData;
    std::vector<char> rawTileData;

    // The result
    std::shared_ptr<MapTile> tile;

    TileTaskData(TileID _tileID, DataSource* _source) :
        tileID(_tileID),
        source(_source) {
    }
};

// NB: with c++14 this should become unique_ptr:
// Moving unique_ptr into closure is a not well supported with c++11
typedef std::shared_ptr<TileTaskData> TileTask;

typedef std::function<void(TileTask)> TileTaskCb;
