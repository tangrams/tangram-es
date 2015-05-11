#include "tileWorker.h"
#include "platform.h"
#include "view/view.h"
#include "style/style.h"

#include <chrono>

TileWorker::TileWorker() {
    m_free = true;
    m_aborted = false;
    m_finished = false;
}

void TileWorker::abort() {
    m_aborted = true;
}

void TileWorker::processTileData(std::unique_ptr<WorkerData> _workerData, 
                                 const std::vector<std::unique_ptr<DataSource>>& _dataSources,
                                 const std::vector<std::unique_ptr<Style>>& _styles,
                                 const View& _view) {

    m_workerData = std::move(_workerData);
    m_free = false;
    m_finished = false;
    m_aborted = false;

    m_future = std::async(std::launch::async, [&]() {
        
        TileID tileID = *(m_workerData->tileID);
        auto& dataSource = _dataSources[m_workerData->dataSourceID];
        
        auto tile = std::shared_ptr<MapTile>(new MapTile(tileID, _view.getMapProjection()));

        if( !(dataSource->hasTileData(tileID)) ) {
            dataSource->setTileData( tileID, dataSource->parse(*tile, m_workerData->rawTileData));
        }

        auto tileData = dataSource->getTileData(tileID);
        
		tile->update(0, _view);

        //Process data for all styles
        for(const auto& style : _styles) {
            if(m_aborted) {
                m_finished = true;
                return std::move(tile);
            }
            if(tileData) {
                style->addData(*tileData, *tile, _view.getMapProjection());
            }
        }
        m_finished = true;
        
        requestRender();
        
        // Return finished tile
        return std::move(tile);

    });

}

std::shared_ptr<MapTile> TileWorker::getTileResult() {

    m_free = true;
    return std::move(m_future.get());
    
}

