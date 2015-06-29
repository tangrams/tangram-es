#include "tileWorker.h"

#include "data/dataSource.h"
#include "platform.h"
#include "tile/mapTile.h"
#include "view/view.h"
#include "tileManager.h"

#include <chrono>

TileWorker::TileWorker(TileManager& _tileManager)
    : m_tileManager(_tileManager) {
    m_aborted = false;
    m_running = false;
}

void TileWorker::abort() {
    m_aborted = true;
}

void TileWorker::process(const StyleSet& _styles)
{
    m_running = true;
    m_aborted = false;

    m_future = std::async(std::launch::async, [&]() {
        while (!m_aborted) {

            auto task = m_tileManager.pollProcessQueue();
            if (!task) {
                // No work left to do.
                break;
            }

            DataSource* dataSource = task->source;
            auto& tile = task->tile;

            std::shared_ptr<TileData> tileData;

            if (task->parsedTileData) {
                // Data has already been parsed!
                tileData = task->parsedTileData;
            } else {
                // Data needs to be parsed
                tileData = dataSource->parse(*tile, task->rawTileData);

                // Cache parsed data with the original data source
                dataSource->setTileData(tile->getID(), tileData);
            }

            if (tileData) {
                // Process data for all styles
                for (const auto& style : _styles) {
                    if (m_aborted) {
                        break;
                    }
                    if(tile->isCanceled()) {
                        break;
                    }
                    style->addData(*tileData, *tile);
                }
            }

            m_tileManager.tileProcessed(std::move(task));
            requestRender();
        }

        m_running = false;
        return true;
    });
}

void TileWorker::drain() {
    m_future.get();
}
