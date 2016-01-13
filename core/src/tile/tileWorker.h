#pragma once

#include <memory>
#include <vector>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>

namespace Tangram {

class DataSource;
class Scene;
class Tile;
class TileTask;
class View;

class TileWorker {

public:

    TileWorker(int _num_worker);

    ~TileWorker();

    void enqueue(std::shared_ptr<TileTask>&& task);

    void stop();

    bool isRunning() const { return m_running; }

    // Check pending-tiles flag. Resets flag on each call..
    // TODO better name checkAndResetPendingTilesFlag?
    bool checkPendingTiles() {
        if (m_pendingTiles) {
            m_pendingTiles = false;
            return true;
        }
        return false;
    }

    void setScene(std::shared_ptr<Scene> _scene) { m_scene = _scene; }

private:

    void run();

    bool m_running;

    std::atomic<bool> m_pendingTiles;

    std::vector<std::thread> m_workers;
    std::condition_variable m_condition;

    std::mutex m_mutex;
    std::vector<std::shared_ptr<TileTask>> m_queue;

    std::shared_ptr<Scene> m_scene;
};

}
