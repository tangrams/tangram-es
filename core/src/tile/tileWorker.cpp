#include "tileWorker.h"

#include "platform.h"
#include "data/dataSource.h"
#include "tile/tileID.h"
#include "tile/tileTask.h"
#include "tile/tileBuilder.h"
#include "tangram.h"
#include "log.h"

#include <algorithm>

#define WORKER_NICENESS 10

namespace Tangram {

TileWorker::TileWorker(int _num_worker) {
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

    std::unique_ptr<TileBuilder> builder;

    while (true) {

        std::shared_ptr<TileTask> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_condition.wait(lock, [&, this]{
                    return !m_running || !m_queue.empty();
                });

            if (instance->tileBuilder) {
                builder = std::move(instance->tileBuilder);
                LOG("Passed new TileBuilder to TileWorker");
            }

            // Check if thread should stop
            if (!m_running) {
                break;
            }

            if (!builder) {
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
                    if (a->source().id() == b->source().id() &&
                        a->sourceGeneration() != b->sourceGeneration()) {
                        return a->sourceGeneration() < b->sourceGeneration();
                    }
                    return a->getPriority() < b->getPriority();
                });

            task = std::move(*it);
            m_queue.erase(it);
        }

        if (task->isCanceled()) {
            continue;
        }

        // const clock_t begin = clock();

        task->process(*builder);

        // float loadTime = (float(clock() - begin) / CLOCKS_PER_SEC) * 1000;
        // LOG("loadTime %s - %f", task->tileID.toString().c_str(), loadTime);

        requestRender();
    }
}

void TileWorker::setScene(std::shared_ptr<Scene>& _scene) {
    for (auto& worker : m_workers) {
        worker->tileBuilder = std::make_unique<TileBuilder>(_scene);
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
