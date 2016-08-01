#pragma once

#include "tile/tileTask.h"
#include "util/jobQueue.h"

#include <memory>
#include <vector>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>

namespace Tangram {

class JobQueue;
class Scene;
class TileBuilder;

class TileWorker : public TileTaskQueue {

public:

    TileWorker(int _num_worker);

    ~TileWorker();

    virtual void enqueue(std::shared_ptr<TileTask>&& task) override;

    void stop();

    bool isRunning() const { return m_running; }

    void setScene(std::shared_ptr<Scene>& _scene);

private:

    struct Worker {
        std::thread thread;
        std::unique_ptr<TileBuilder> tileBuilder;
    };

    void run(Worker* instance);

    bool m_running;

    std::vector<std::unique_ptr<Worker>> m_workers;

    std::condition_variable m_condition;

    std::mutex m_mutex;
    std::vector<std::shared_ptr<TileTask>> m_queue;
};

}
