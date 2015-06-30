#include "tileWorker.h"

#include "data/dataSource.h"
#include "platform.h"
#include "tile/mapTile.h"
#include "view/view.h"
#include "tileManager.h"
#include "style/style.h"
#include "scene/scene.h"


TileWorker::TileWorker(TileManager& _tileManager, int _num_worker)
    : m_tileManager(_tileManager) {
    m_running = true;

    for (int i = 0; i < _num_worker; i++) {
        m_workers.emplace_back(&TileWorker::run, this);
    }
}

TileWorker::~TileWorker(){
    if (m_running) {
        stop();
    }
}

void TileWorker::run() {
    while (true) {

        TileTask task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [&, this]{
                    return !m_running || !m_queue.empty();
                });

            if (!m_running && m_queue.empty())
                break;

            task = m_queue.front();
            m_queue.pop_front();
        }
        if (task->tile->isCanceled())
            continue;

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
            for (const auto& style : m_tileManager.getScene()->getStyles()) {
                if (!m_running) {
                    continue;
                }
                if(tile->isCanceled()) {
                    continue;
                }
                style->addData(*tileData, *tile);
            }
        }

        m_tileManager.tileProcessed(std::move(task));
        requestRender();
    }
}

void TileWorker::enqueue(TileTask task) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_running)
            return;

        m_queue.push_back(task);
    }
    m_condition.notify_one();
}

void TileWorker::stop() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_running = false;
    }

    m_condition.notify_all();

    for(std::thread &worker: m_workers)
        worker.join();
}
