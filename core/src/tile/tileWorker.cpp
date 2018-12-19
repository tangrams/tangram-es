#include "tile/tileWorker.h"

#include "data/tileSource.h"
#include "log.h"
#include "map.h"
#include "platform.h"
#include "scene/scene.h"
#include "tile/tileBuilder.h"
#include "tile/tileID.h"
#include "tile/tileTask.h"

#include <algorithm>

#define WORKER_NICENESS 10

namespace Tangram {

TileWorker::TileWorker(Platform& _platform, int _numWorker) : m_platform(_platform) {
    m_running = true;

    for (int i = 0; i < _numWorker; i++) {
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

    std::unique_ptr<TileBuilder> builder;

    while (true) {

        std::shared_ptr<TileTask> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_condition.wait(lock, [&] {
                return (!m_queue.empty() && m_sceneComplete) || !m_running || instance->tileBuilder;
            });

            if (instance->tileBuilder) {
                LOGTInit();
                builder = std::move(instance->tileBuilder);
                builder->init();
                LOGT("Took init of TileBuilder");
            }
            // Check if thread should stop
            if (!m_running) {
                break;
            }

            if (!builder || !m_sceneComplete) {
                if (builder) LOGTO("Waiting for Scene to become ready");
                continue;
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
                    if (a->isProxy() != b->isProxy()) {
                        return !a->isProxy();
                    }
                    if (a->sourceId() == b->sourceId() &&
                        a->sourceGeneration() != b->sourceGeneration()) {
                        return a->sourceGeneration() < b->sourceGeneration();
                    }
                    return a->getPriority() < b->getPriority();
                });

            task = std::move(*it);
            m_queue.erase(it);
        }

        if (task->isCanceled()) { continue; }

        LOGTInit(">>> process %s", task->tileId().toString().c_str());
        task->process(*builder);
        LOGT("<<< process %s", task->tileId().toString().c_str());

        m_platform.requestRender();
    }
}

void TileWorker::setScene(Scene& _scene) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        for (auto& worker : m_workers) {
            worker->tileBuilder = std::make_unique<TileBuilder>(_scene);
        }
        m_condition.notify_all();
    }
}

void TileWorker::enqueue(std::shared_ptr<TileTask> task) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_running) { return; }
        LOGTO("--- %d enqueue %s", m_queue.size()+1, task->tileId().toString().c_str());
        m_queue.push_back(std::move(task));

        m_condition.notify_all();
    }
}

void TileWorker::startJobs() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_sceneComplete = true;

        LOGTO("Poking TileWorker - enqueued %d", m_queue.size());
        if (!m_running || m_queue.empty()) { return; }

        m_condition.notify_all();
    }
}

void TileWorker::stop() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_running = false;
        m_condition.notify_all();
    }

    for (auto& worker : m_workers) {
        worker->thread.join();
    }

    m_queue.clear();
}

}
