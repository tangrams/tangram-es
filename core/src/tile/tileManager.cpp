#include "tileManager.h"
#include "scene/scene.h"
#include "tile/mapTile.h"
#include "view/view.h"

#include <chrono>

TileManager::TileManager() {
    
    // Instantiate workers
    for (int i = 0; i < MAX_WORKERS; i++) {
        m_freeWorkers.push_back(std::unique_ptr<TileWorker>(new TileWorker()));
    }
}

TileManager::TileManager(TileManager&& _other) :
    m_view(std::move(_other.m_view)),
    m_tileSet(std::move(_other.m_tileSet)),
    m_dataSources(std::move(_other.m_dataSources)) {
}

TileManager::~TileManager() {
    m_dataSources.clear();
    m_tileSet.clear();
    for (auto& worker : m_busyWorkers) {
        worker->abort();
    }
}

bool TileManager::updateTileSet() {
    
    bool tileSetChanged = false;
    
    // Check if any incoming tiles are finished
    {
        auto busyWorkersIter = m_busyWorkers.begin();
        
        while (busyWorkersIter != m_busyWorkers.end()) {
            
            auto& worker = *busyWorkersIter;
            
            if (worker->isFinished()) {
                auto tile = worker->getTileResult();
                const TileID& id = tile->getID();
                logMsg("Tile [%d, %d, %d] finished loading\n", id.z, id.x, id.y);
                std::swap(m_tileSet[id], tile);
                cleanProxyTiles(id);
                tileSetChanged = true;
                //auto freedWorkerIter = busyWorkersIter++;
                //m_freeWorkers.splice(m_freeWorkers.end(), m_busyWorkers, freedWorkerIter);
                m_freeWorkers.push_back(std::move(worker));
                busyWorkersIter = m_busyWorkers.erase(busyWorkersIter);
            } else {
                ++busyWorkersIter;
            }
        }
    }
    
    if (! (m_view->changedSinceLastCheck() || tileSetChanged) ) {
        // No new tiles have come into view and no tiles have finished loading, 
        // so the tileset is unchanged
        return false;
    }
    
    const std::set<TileID>& visibleTiles = m_view->getVisibleTiles();
    
    // Loop over visibleTiles and add any needed tiles to tileSet
    {
        auto setTilesIter = m_tileSet.begin();
        auto visTilesIter = visibleTiles.begin();
        
        while (visTilesIter != visibleTiles.end()) {
            
            if (setTilesIter == m_tileSet.end() || *visTilesIter < setTilesIter->first) {
                // tileSet is missing an element present in visibleTiles
                addTile(*visTilesIter);
                tileSetChanged = true;
                ++visTilesIter;
            } else if (setTilesIter->first < *visTilesIter) {
                // visibleTiles is missing an element present in tileSet (handled below)
                ++setTilesIter;
            } else {
                // tiles in both sets match, move on
                ++setTilesIter;
                ++visTilesIter;
            }
        }
    }
    
    // Loop over tileSet and remove any tiles that are neither visible nor proxies
    {
        auto setTilesIter = m_tileSet.begin();
        auto visTilesIter = visibleTiles.begin();
        
        while (setTilesIter != m_tileSet.end()) {
            
            if (visTilesIter == visibleTiles.end() || setTilesIter->first < *visTilesIter) {
                // visibleTiles is missing an element present in tileSet
                if (setTilesIter->second->getProxyCounter() <= 0) {
                    removeTile(setTilesIter);
                    tileSetChanged = true;
                } else {
                    ++setTilesIter;
                }
            } else if (*visTilesIter < setTilesIter->first) {
                // tileSet is missing an element present in visibleTiles (shouldn't occur)
                ++visTilesIter;
            } else {
                // tiles in both sets match, move on
                ++setTilesIter;
                ++visTilesIter;
            }
        }
    }
    
    // Dispatch workers for queued tiles
    {
        auto freeWorkersIter = m_freeWorkers.begin();
        auto queuedTilesIter = m_queuedTiles.begin();
        
        while (freeWorkersIter != m_freeWorkers.end() && queuedTilesIter != m_queuedTiles.end()) {
            
            TileID id = *queuedTilesIter;
            auto& worker = *freeWorkersIter;
            
            worker->load(id, m_dataSources, m_scene->getStyles(), *m_view);
            m_busyWorkers.push_back(std::move(worker));
            
            freeWorkersIter = m_freeWorkers.erase(freeWorkersIter);
            queuedTilesIter = m_queuedTiles.erase(queuedTilesIter);
        }
    }
    
    return tileSetChanged;
}

void TileManager::addTile(const TileID& _tileID) {
    
    std::shared_ptr<MapTile> tile(new MapTile(_tileID, m_view->getMapProjection()));
    m_tileSet[_tileID] = std::move(tile);
    
    //Add Proxy if corresponding proxy MapTile ready
    updateProxyTiles(_tileID, m_view->isZoomIn());
    
    // Queue tile for workers
    m_queuedTiles.push_back(_tileID);
    
}

void TileManager::removeTile(std::map< TileID, std::shared_ptr<MapTile> >::iterator& _tileIter) {
    
    const TileID& id = _tileIter->first;
    
    // Remove tile from queue, if present
    m_queuedTiles.remove(id);
    
    // If a worker is loading this tile, abort it
    for (const auto& worker : m_busyWorkers) {
        if (worker->getTileID() == id) {
            worker->abort();
        }
    }
    
    // Remove tile from set
    _tileIter = m_tileSet.erase(_tileIter);
    
}

void TileManager::updateProxyTiles(const TileID& _tileID, bool _zoomingIn) {
    if (_zoomingIn) {
        // zoom in - add parent
        TileID parent = _tileID.getParent();
        if (parent.isValid() && m_tileSet.find(parent) != m_tileSet.end()) {
            m_tileSet[parent]->incProxyCounter();
        }
    } else {
        for(int i = 0; i < 4; i++) {
            TileID child = _tileID.getChild(i);
            if(child.isValid(m_view->s_maxZoom) && m_tileSet.find(child) != m_tileSet.end()) {
                m_tileSet[child]->incProxyCounter();
            }
        }
    }
}

void TileManager::cleanProxyTiles(const TileID& _tileID) {
    // check if parent proxy is present
    TileID parent = _tileID.getParent();
    if (parent.isValid() && m_tileSet.find(parent) != m_tileSet.end()) {
        m_tileSet[parent]->decProxyCounter();
    }
    
    // check if child proxies are present
    for(int i = 0; i < 4; i++) {
        TileID child = _tileID.getChild(i);
        if(child.isValid(m_view->s_maxZoom) && m_tileSet.find(child) != m_tileSet.end()) {
            m_tileSet[child]->decProxyCounter();
        }
    }
}
