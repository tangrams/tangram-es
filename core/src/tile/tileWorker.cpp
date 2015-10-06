#include "tileWorker.h"

#include "platform.h"
#include "tile/tile.h"
#include "view/view.h"
#include "tileManager.h"
#include "scene/scene.h"
#include "scene/styleContext.h"

#include <algorithm>

#define WORKER_NICENESS 10

namespace Tangram {

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
                [](const auto& a) { return a->tile->isCanceled(); });

            m_queue.erase(removes, m_queue.end());

            if (m_queue.empty()) {
                continue;
            }

            // Pop highest priority tile from queue
            auto it = std::min_element(m_queue.begin(), m_queue.end(),
                [](const auto& a, const auto& b) {
                    if (a->tile->isVisible() != b->tile->isVisible()) {
                        return a->tile->isVisible();
                    }
                    return a->tile->getPriority() < b->tile->getPriority();
                });

            task = std::move(*it);
            m_queue.erase(it);
        }

        if (task->tile->isCanceled()) {
            continue;
        }

        auto tileData = task->process();

        // NB: Save shared reference to Scene while building tile
        auto scene = m_tileManager.getScene();

        context.initFunctions(*scene);

        if (tileData) {
            task->tile->build(context, *scene, *tileData, *task->source);
        }

        m_tileManager.tileProcessed(std::move(task));
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
