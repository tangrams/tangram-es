#include "tileWorker.h"

#include "platform.h"
#include "tile/tile.h"
#include "view/view.h"
#include "scene/scene.h"
#include "scene/styleContext.h"
#include "tile/tileManager.h"
#include "tile/tileID.h"
#include "tile/tileTask.h"

#include <algorithm>

#define WORKER_NICENESS 5

namespace Tangram {

TileWorker::TileWorker(TileManager& _tileManager, int _num_worker)
    : m_tileManager(_tileManager) {
    m_running = true;

    for (int i = 0; i < _num_worker; i++) {
        auto worker = std::make_unique<Worker>();
        worker->thread = std::thread(&TileWorker::run, this, worker.get());
        m_workers.push_back(std::move(worker));
    }
}

TileWorker::~TileWorker(){
    if (m_running) {
        stop();
    }
}

void TileWorker::run(Worker* instance) {

    setCurrentThreadPriority(WORKER_NICENESS);

    std::unique_ptr<StyleContext> context;

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
                    if (a->tile->sourceID() == b->tile->sourceID() &&
                        a->tile->sourceGeneration() != b->tile->sourceGeneration()) {
                        return a->tile->sourceGeneration() < b->tile->sourceGeneration();
                    }
                    return a->tile->getPriority() < b->tile->getPriority();
                });

            task = std::move(*it);
            m_queue.erase(it);
        }

        if (task->tile->isCanceled()) {
            continue;
        }

        if (instance->styleContext) {
            context = std::move(instance->styleContext);
            LOG("Passed new StyleContext to TileWorker");
        }

        if (!context) {
            LOGE("Missing Scene/StyleContext in TileWorker!");
            continue;
        }

        auto t1 = clock();

        auto tileData = task->process();

        auto t2 = clock();

        if (tileData) {
            task->tile->build(*context, *tileData, *task->source);
            auto t3 = clock();

            float parseTime = (float(t2 - t1) / CLOCKS_PER_SEC) * 1000;
            float buildTime = (float(t3 - t2) / CLOCKS_PER_SEC) * 1000;
            float allTime = (float(t3 - t1) / CLOCKS_PER_SEC) * 1000;
            LOG("%s - parse:%f\t build:%f\t all:%f", task->tile->getID().toString().c_str(),
                parseTime, buildTime, allTime);

        }

        m_tileManager.tileProcessed(std::move(task));

        requestRender();
    }
}

void TileWorker::setScene(const Scene& _scene) {
    for (auto& worker : m_workers) {
        auto styleContext = std::make_unique<StyleContext>();
        styleContext->setScene(_scene);

        worker->styleContext = std::move(styleContext);
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

    for (auto& worker : m_workers) {
        worker->thread.join();
    }
}

}
