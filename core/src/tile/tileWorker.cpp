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

void TileWorker::processTileData(std::unique_ptr<TileTask> _task,
                                 const std::vector<std::unique_ptr<Style>>& _styles,
                                 const View& _view) {

    m_task = std::move(_task);
    m_free = false;
    m_finished = false;
    m_aborted = false;

    m_future = std::async(std::launch::async, [&]() {
        
        const TileID& tileID = m_task->tileID;
        DataSource* dataSource = m_task->source;
        
        auto tile = std::shared_ptr<MapTile>(new MapTile(tileID, _view.getMapProjection()));

        std::shared_ptr<TileData> tileData;

        if (m_task->parsedTileData) {
            // Data has already been parsed!
            tileData = m_task->parsedTileData;
        } else {
            // Data needs to be parsed
            tileData = dataSource->parse(*tile, m_task->rawTileData);

            // Cache parsed data with the original data source
            dataSource->setTileData(tileID, tileData);
        }
        
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

