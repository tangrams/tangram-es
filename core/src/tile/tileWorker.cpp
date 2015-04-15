#include "tileWorker.h"
#include "platform.h"
#include "view/view.h"
#include "style/style.h"

#include <chrono>
#include <sstream>

TileWorker::TileWorker() {
    m_free = true;
    m_aborted = false;
    m_finished = false;
}

void TileWorker::abort() {
    m_aborted = true;
}

void TileWorker::processTileData(const WorkerData& _workerData, 
                                 const std::vector<std::unique_ptr<DataSource>>& _dataSources,
                                 const std::vector<std::unique_ptr<Style>>& _styles,
                                 const View& _view) {

    m_workerData.reset(new WorkerData(std::move(_workerData)));
    m_free = false;
    m_finished = false;
    m_aborted = false;

    m_future = std::async(std::launch::async, [&]() {
        
        TileID tileID = *(m_workerData->tileID);
        const char* rawData = m_workerData->rawTileData;
        int dataSize = m_workerData->dataSize;
        auto& dataSource = _dataSources[m_workerData->dataSourceID];
        
        auto tile = std::shared_ptr<MapTile>(new MapTile(tileID, _view.getMapProjection()));

        if( !(dataSource->hasTileData(tileID)) ) {
            dataSource->setTileData( tileID, dataSource->parse(*tile, rawData, dataSize));
            delete rawData;
        }

        auto tileData = dataSource->getTileData(tileID);
        
        //Process data for all styles
        for(const auto& style : _styles) {
            if(m_aborted) {
                m_finished = true;
                return std::move(tile);
            }
            if(tileData) {
                style->addData(*tileData, *tile, _view.getMapProjection());
            }
            tile->update(0, *style, _view);
        }
        m_finished = true;

        // Return finished tile
        return std::move(tile);

    });

}

std::shared_ptr<MapTile> TileWorker::getTileResult() {

    m_free = true;
    return std::move(m_future.get());
    
}

