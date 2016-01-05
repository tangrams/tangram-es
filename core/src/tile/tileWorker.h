#pragma once

#include <memory>
#include <vector>
#include <condition_variable>
#include <thread>
#include <mutex>

namespace Tangram {

class TileManager;
class DataSource;
class StyleContext;
class Scene;
class Tile;
class TileTask;
class View;

class TileWorker {

public:

    TileWorker(TileManager& _tileManager, int _num_worker);

    ~TileWorker();

    void enqueue(std::shared_ptr<TileTask>&& task);

    void stop();

    bool isRunning() const { return m_running; }

    void setScene(const Scene& _scene);

private:

    struct Worker {
        std::thread thread;
        std::unique_ptr<StyleContext> styleContext;
    };

    void run(Worker* instance);

    TileManager& m_tileManager;

    bool m_running;

    std::vector<std::unique_ptr<Worker>> m_workers;
    std::condition_variable m_condition;

    std::mutex m_mutex;
    std::vector<std::shared_ptr<TileTask>> m_queue;
};

}
