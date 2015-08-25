#pragma once

#include "tile/tileID.h"
#include "tileTask.h"

#include <memory>
#include <future>
#include <memory>
#include <vector>
#include <condition_variable>
#include <thread>
#include <mutex>

namespace Tangram {

class TileManager;
class DataSource;
class Tile;
class View;

class TileWorker {
    
public:
    
    TileWorker(TileManager& _tileManager, int _num_worker);

    ~TileWorker();
    
    void enqueue(std::shared_ptr<TileTask>&& task);
    
    void stop();
    
    bool isRunning() const { return m_running; }
    
private:

    void run();
    
    TileManager& m_tileManager;
    
    bool m_running;
    
    std::vector<std::thread> m_workers;
    std::condition_variable m_condition;

    std::mutex m_mutex;
    std::vector<std::shared_ptr<TileTask>> m_queue;
};

}
