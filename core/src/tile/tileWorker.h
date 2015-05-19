#pragma once

#include <memory>
#include <future>

#include "util/tileID.h"
#include "data/dataSource.h"
#include "mapTile.h"

struct TileTask {

    TileID tileID;

    // Only one of either parsedTileData or rawTileData will be non-empty for a given task.
    // If parsedTileData is non-empty, then the data for this tile was previously fetched
    // and parsed. Otherwise rawTileData will be non-empty, indicating that the data needs 
    // to be parsed using the given DataSource. 
    std::shared_ptr<TileData> parsedTileData;
    std::vector<char> rawTileData;
    DataSource* source;

    TileTask() : tileID(NOT_A_TILE) {
    }

    TileTask(std::vector<char>&& _rawTileData, const TileID& _tileID, DataSource* _source) :
        tileID(_tileID),
        rawTileData(std::move(_rawTileData)),
        source(_source) {
    }

    TileTask(std::shared_ptr<TileData> _tileData, const TileID& _tileID, DataSource* _source) :
        tileID(_tileID),
        parsedTileData(_tileData),
        source(_source) {       
    }

    TileTask(TileTask&& _other) :
        tileID(_other.tileID),
        parsedTileData(std::move(_other.parsedTileData)),
        rawTileData(std::move(_other.rawTileData)),
        source(std::move(_other.source)) {
    }

};

class TileWorker {
    
public:
    
    TileWorker();
    
    void processTileData(std::unique_ptr<TileTask> _task,
                         const std::vector<std::unique_ptr<Style>>& _styles,
                         const View& _view);
    
    void abort();
    
    bool isFinished() const { return m_finished; }

    bool isFree() const { return m_free; }
    
    const TileID& getTileID() const { return m_task->tileID; }
    
    std::shared_ptr<MapTile> getTileResult();
    
private:
    
    bool m_free;
    bool m_aborted;
    bool m_finished;
    
    std::unique_ptr<TileTask> m_task;
    std::future< std::shared_ptr<MapTile> > m_future;
};

