#pragma once

#include "data/tileData.h"
#include "util/tileID.h"
#include "style/style.h"
#include "tileTask.h"

#include <memory>
#include <future>
#include <memory>
#include <future>

class TileManager;
class DataSource;
class MapTile;
class View;

class TileWorker {
    
public:
    
    TileWorker(TileManager& _tileManager);
    
    void process(const StyleSet& _styles);
    
    void abort();
    
    bool isRunning() const { return m_running; }

    void drain();
    
private:
    
    TileManager& m_tileManager;
    
    bool m_aborted;
    bool m_running;
    
    std::future<bool> m_future;
};

