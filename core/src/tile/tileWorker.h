#pragma once

#include <memory>
#include <future>

#include "util/tileID.h"
#include "data/dataSource.h"
#include "mapTile.h"

struct WorkerData {
    const char* rawTileData;
    int dataSize;
    int dataSourceID;
    std::unique_ptr<TileID> tileID;

    WorkerData() {
        tileID.reset(new TileID(NOT_A_TILE));
    }

    WorkerData(const char* _rawTileData, const int _dataSize, const TileID& _tileID, const int _dataSourceID) :
                                                                rawTileData(_rawTileData), dataSize(_dataSize),
                                                                dataSourceID(_dataSourceID) {
        tileID.reset(new TileID(_tileID));
    }

    WorkerData(const WorkerData&& _other) : rawTileData(_other.rawTileData) {
        tileID.reset(new TileID(*(_other.tileID)));
        dataSourceID = std::move(_other.dataSourceID);
        dataSize = std::move(_other.dataSize);
    }

    bool operator==(const WorkerData& _rhs) const {
         return tileID == _rhs.tileID;
    }

};

class TileWorker {
    
public:
    
    TileWorker();
    
    void processTileData(const WorkerData& _workerData, 
                         const std::vector<std::unique_ptr<DataSource>>& _dataSources,
                         const std::vector<std::unique_ptr<Style>>& _styles,
                         const View& _view);
    
    void abort();
    
    bool isFinished() const { return m_finished; }

    bool isFree() const { return m_free; }
    
    const TileID& getTileID() const { return m_workerData->tileID ? *(m_workerData->tileID) : NOT_A_TILE; }
    
    std::shared_ptr<MapTile> getTileResult();
    
private:
    
    std::unique_ptr<WorkerData> m_workerData;
    
    bool m_free;
    bool m_aborted;
    bool m_finished;
    
    std::future< std::shared_ptr<MapTile> > m_future;
};
