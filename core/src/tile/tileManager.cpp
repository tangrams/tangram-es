#include "tileManager.h"
#include "scene/scene.h"
#include "tile/mapTile.h"
#include "view/view.h"

#include <chrono>

TileManager::TileManager() {
}

TileManager::TileManager(TileManager&& _other) :
    m_view(std::move(_other.m_view)),
    m_tileSet(std::move(_other.m_tileSet)),
    m_dataSources(std::move(_other.m_dataSources)),
    m_incomingTiles(std::move(_other.m_incomingTiles)) {
}

TileManager::~TileManager() {
    m_dataSources.clear();
    m_tileSet.clear();
    m_incomingTiles.clear();
}

bool TileManager::updateTileSet() {
    
    bool tileSetChanged = false;
    
    // Check if any incoming tiles are finished
    {
        auto incomingTilesIter = m_incomingTiles.begin();
        
        while (incomingTilesIter != m_incomingTiles.end()) {
            
            auto& tileFuture = *incomingTilesIter;
            std::chrono::milliseconds span (0);
            
            // check if future's shared state is null
            // i.e. The tile it was supposed to hold, is no longer part of m_tileSet and hence no longer loaded
            if (tileFuture.wait_for(span) == std::future_status::ready) {
                auto tile = tileFuture.get();
                // possible a tile is deleted by the main thread before it gets finished
                if (tile) {
                    const TileID& id = tile->getID();
                    logMsg("Tile [%d, %d, %d] finished loading\n", id.z, id.x, id.y);
                    m_tileSet[id] = tile;
                    // tile is now loaded, removed its proxies
                    cleanProxyTiles(tile->getID());
                    tileSetChanged = true;
                }
                incomingTilesIter = m_incomingTiles.erase(incomingTilesIter);
            } else {
                ++incomingTilesIter;
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
    
    return tileSetChanged;
}

void TileManager::addTile(const TileID& _tileID) {
    
    std::shared_ptr<MapTile> tile(new MapTile(_tileID, m_view->getMapProjection()));
    m_tileSet[_tileID] = tile;
    
    //Add Proxy if corresponding proxy MapTile ready
    updateProxyTiles(_tileID, m_view->isZoomIn());

    std::future< std::shared_ptr<MapTile> > incoming = std::async(std::launch::async, [&](TileID _id) {
        
        // Check if tile to be loaded is still required! (either not culled from m_tileSet and not logically deleted)
        // if not set the shared state of this async's future to "null"
        if (m_tileSet.find(_id) != m_tileSet.end()) {
            auto tile = m_tileSet[_id];
            
            // Now Start fetching new tile
            for (const auto& dataSource : m_dataSources) {
                
                logMsg("Loading tile [%d, %d, %d]\n", _id.z, _id.x, _id.y);
                if ( ! dataSource->loadTileData(*tile)) {
                    logMsg("ERROR: Loading failed for tile [%d, %d, %d]\n", _id.z, _id.x, _id.y);
                }
                
                auto tileData = dataSource->getTileData(_id);
                
                for (auto& style : m_scene->getStyles()) {
                    if (tileData) {
                        style->addData(*tileData, *tile, m_view->getMapProjection());
                    }
                }
            }
            
            return tile;
        } else {
            // tile was deleted (went out off view) before it could load its data
            // remove its proxies also
            cleanProxyTiles(_id);
            return std::shared_ptr<MapTile> (nullptr);
        }
    
    }, _tileID);
    
    m_incomingTiles.push_back(std::move(incoming));
    
}

void TileManager::removeTile(std::map< TileID, std::shared_ptr<MapTile> >::iterator& _tileIter) {
    
    _tileIter = m_tileSet.erase(_tileIter);

    // TODO: if tile is being loaded, cancel loading; For now they continue to load
    // and will be culled the next time that updateTileSet is called
    
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
