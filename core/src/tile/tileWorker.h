#pragma once

#include <memory>
#include <future>

#include "util/tileID.h"
#include "data/dataSource.h"
#include "mapTile.h"

struct WorkerData {
    std::vector<char> rawTileData;
    std::unique_ptr<TileID> tileID;
    DataSource* source;

    WorkerData() {
        tileID.reset(new TileID(NOT_A_TILE));
    }

    WorkerData(std::vector<char>&& _rawTileData, const TileID& _tileID, DataSource* _source) :
        rawTileData(std::move(_rawTileData)),
        source(_source) {
            
        tileID.reset(new TileID(_tileID));
        
    }

    WorkerData(WorkerData&& _other) :
        rawTileData(std::move(_other.rawTileData)),
        source(std::move(_other.source)) {
            
        tileID.reset(new TileID(*(_other.tileID)));
        
    }

    WorkerData& operator=(WorkerData&& _other) {
        rawTileData = std::move(_other.rawTileData);
        tileID.reset(new TileID(*(_other.tileID)));
        source = std::move(_other.source);
        return *this;
    }

};

class TileWorker {
    
public:
    
    TileWorker();
    
    void processTileData(std::unique_ptr<WorkerData> _workerData,
                         const std::vector<std::unique_ptr<Style>>& _styles,
                         const View& _view);
    
    void abort();
    
    bool isFinished() const { return m_finished; }

    bool isFree() const { return m_free; }
    
    const TileID& getTileID() const { return m_workerData->tileID ? *(m_workerData->tileID) : NOT_A_TILE; }
    
    std::shared_ptr<MapTile> getTileResult();
    
private:
    
    bool m_free;
    bool m_aborted;
    bool m_finished;
    
    std::unique_ptr<WorkerData> m_workerData;
    std::future< std::shared_ptr<MapTile> > m_future;
};

