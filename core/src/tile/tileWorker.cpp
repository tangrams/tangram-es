#include "tileWorker.h"

#include "platform.h"
#include "data/dataSource.h"
#include "tile/tile.h"
#include "view/view.h"
#include "scene/scene.h"
#include "tile/tileID.h"
#include "tile/tileTask.h"
#include "tile/tileBuilder.h"
#include "tangram.h"

#include <algorithm>

#define WORKER_NICENESS 10

namespace Tangram {

TileWorker::TileWorker(int _num_worker) {
    m_running = true;
    m_pendingTiles = false;

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

void disposeBuilder(std::unique_ptr<TileBuilder> _builder) {
    if (_builder) {
        Tangram::runOnMainLoop([b = std::shared_ptr<TileBuilder>(std::move(_builder))](){});
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
                disposeBuilder(std::move(builder));

                builder = std::move(instance->tileBuilder);
                LOG("Passed new StyleContext to TileWorker");
            }


            // Check if thread should stop
            if (!m_running) {
                disposeBuilder(std::move(builder));
                break;
            }

            if (!builder) {
                LOGE("Missing Scene/StyleContext in TileWorker!");
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

        auto tileData = task->source().parse(*task, *builder->scene().mapProjection());

        // const clock_t begin = clock();

        if (tileData) {

            auto&& tile = builder->build(task->tileId(), *tileData, task->source());

            // Mark task as ready
            task->setTile(std::move(tile));

            // float loadTime = (float(clock() - begin) / CLOCKS_PER_SEC) * 1000;
            // LOG("loadTime %s - %f", task->tile()->getID().toString().c_str(), loadTime);
        } else {
            task->cancel();
        }

        m_pendingTiles = true;

        requestRender();
    }
}

void TileWorker::setScene(std::shared_ptr<Scene>& _scene) {
    for (auto& worker : m_workers) {
        auto tileBuilder = std::make_unique<TileBuilder>();
        tileBuilder->setScene(_scene);

        worker->tileBuilder = std::move(tileBuilder);
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
