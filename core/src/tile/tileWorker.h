#pragma once

#include "tile/tileTask.h"
#include "util/jobQueue.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace Tangram {

class JobQueue;
class Platform;
class Scene;
class TileBuilder;

class TileWorker : public TileTaskQueue {

public:

    TileWorker(Platform& _platform, int _numWorker);

    ~TileWorker();

    virtual void enqueue(std::shared_ptr<TileTask> task) override;

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

    Platform& m_platform;
};

}
