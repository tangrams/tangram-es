#include "tileWorker.h"

#include "platform.h"
#include "data/dataSource.h"
#include "tile/tile.h"
#include "view/view.h"
#include "scene/scene.h"
#include "scene/styleContext.h"
#include "tile/tileID.h"
#include "tile/tileTask.h"

#include <algorithm>

#define WORKER_NICENESS 10

namespace Tangram {

TileWorker::TileWorker(int _num_worker) {
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

    setCurrentThreadPriority(WORKER_NICENESS);

    StyleContext context;

    while (true) {

        std::shared_ptr<TileTask> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_condition.wait(lock, [&, this]{
                    return !m_running || !m_queue.empty();
                });

            // Check if thread should stop
            if (!m_running) {
                break;
            }

            // Remove all canceled tasks
            auto removes = std::remove_if(m_queue.begin(), m_queue.end(),
                                          [](const auto& a) { return a->isCanceled(); });

            m_queue.erase(removes, m_queue.end());

            if (m_queue.empty()) {
                continue;
            }

            // Pop highest priority tile from queue
            auto it = std::min_element(m_queue.begin(), m_queue.end(),
                [](const auto& a, const auto& b) {
                    // if (a->visible != b->visible) {
                    //     return a->visible;
                    // }
                    if (a->source().id() == b->source().id() &&
                        a->sourceGeneration() != b->sourceGeneration()) {
                        return a->sourceGeneration() < b->sourceGeneration();
                    }
                    return a->getPriority() < b->getPriority();
                });

            task = std::move(*it);
            m_queue.erase(it);
        }

        if (task->isCanceled()) { continue; }

        // Save shared reference to Scene while building tile
        // FIXME: Scene could be released on Worker-Thread and
        // therefore call unsafe glDelete* functions...
        auto scene = m_scene;
        if (!scene) { continue; }

        auto tileData = task->process(*scene->mapProjection());

        const clock_t begin = clock();

        context.initFunctions(*scene);

        if (tileData) {
            auto tile = std::make_shared<Tile>(task->tileId(),
                                               *scene->mapProjection(),
                                               &task->source());

            tile->build(context, *scene, *tileData, task->source());

            // Mark task as ready
            task->setTile(std::move(tile));

            float loadTime = (float(clock() - begin) / CLOCKS_PER_SEC) * 1000;
            LOG("loadTime %s - %f", task->tile()->getID().toString().c_str(), loadTime);
        } else {
            task->cancel();
        }

        m_pendingTiles = true;

        requestRender();
    }
}

void TileWorker::enqueue(std::shared_ptr<TileTask>&& task) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_running) {
            return;
        }
        m_queue.push_back(std::move(task));
    }
    m_condition.notify_one();
}

void TileWorker::stop() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_running = false;
    }

    m_condition.notify_all();

    for (std::thread &worker: m_workers) {
        worker.join();
    }
}

}
