#include "tileWorker.h"
#include "platform.h"
#include "view/view.h"
#include "style/style.h"

#include <chrono>

TileWorker::TileWorker() {
    m_tileID.reset(new TileID(NOT_A_TILE));
    m_free = true;
    m_aborted = false;
    m_finished = false;
}

void TileWorker::abort() {
    m_aborted = true;
}

void TileWorker::load(const TileID &_tile,
                      const std::vector<std::unique_ptr<DataSource>> &_dataSources,
                      const std::vector<std::unique_ptr<Style>> &_styles,
                      const View& _view) {
    
    m_tileID.reset(new TileID(_tile));
    m_free = false;
    m_finished = false;
    m_aborted = false;
    
    m_future = std::async(std::launch::async, [&](const TileID& _id) {
        
        auto tile = std::shared_ptr<MapTile>(new MapTile(_id, _view.getMapProjection()));
        
        // Fetch tile data from data sources
        logMsg("Loading Tile [%d, %d, %d]\n", _id.z, _id.x, _id.y);
        for (const auto& dataSource : _dataSources) {
            if (m_aborted) {
                m_finished = true;
                return std::move(tile); // Early return
            }
            if (! dataSource->loadTileData(*tile)) {
                logMsg("ERROR: Loading failed for tile [%d, %d, %d]\n", _id.z, _id.x, _id.y);
                continue;
            }
            
            auto tileData = dataSource->getTileData(_id);
            
            // Process data for all styles
            for (const auto& style : _styles) {
                if (m_aborted) {
                    m_finished = true;
                    return std::move(tile); // Early return
                }
                if (tileData) {
                    style->addData(*tileData, *tile, _view.getMapProjection());
                }
                tile->update(0, *style, _view);
            }
        }
        
        m_finished = true;
        
        requestRender();
        
        // Return finished tile
        return std::move(tile);
                                        
    }, *m_tileID);
    
}

std::shared_ptr<MapTile> TileWorker::getTileResult() {

    m_free = true;
    return std::move(m_future.get());
    
}

