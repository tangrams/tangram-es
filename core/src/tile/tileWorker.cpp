#include "tileWorker.h"

#include "data/dataSource.h"
#include "platform.h"
#include "tile/mapTile.h"
#include "view/view.h"

TileWorker::TileWorker() {
    m_free = true;
    m_aborted = false;
    m_finished = false;
}

void TileWorker::abort() {
    m_aborted = true;
}

void TileWorker::processTileData(TileTask _task,
                                 const StyleSet& _styles,
                                 const View& _view)
{
    m_task = std::move(_task);
    m_free = false;
    m_finished = false;
    m_aborted = false;

    m_future = std::async(std::launch::async, [&]() {
        
        DataSource* dataSource = m_task->source;
        auto& tile = m_task->tile;
        
        std::shared_ptr<TileData> tileData;

        if (m_task->parsedTileData) {
            // Data has already been parsed!
            tileData = m_task->parsedTileData;
        } else {
            // Data needs to be parsed
            tileData = dataSource->parse(*tile, m_task->rawTileData);

            // Cache parsed data with the original data source
            dataSource->setTileData(tile->getID(), tileData);
        }

        if (tileData) {
            tile->update(0, _view);
        
            //Process data for all styles
            for(const auto& style : _styles) {
                if (m_aborted || tile->state() == MapTile::Canceled) {
                    m_finished = true;
                    return false;
                }
                style->addData(*tileData, *tile);
            }
        }

        m_finished = true;
        requestRender();

        return true;
    });
}

void TileWorker::drain() {
  m_future.get();
  m_free = true;
}

std::shared_ptr<MapTile> TileWorker::getTileResult() {
    m_future.get();
    m_free = true;
    return std::move(m_task->tile);
}

