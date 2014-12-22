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
            if(!tileFuture.valid()) {
                incomingTilesIter = m_incomingTiles.erase(incomingTilesIter);
            }
            
            else if (tileFuture.wait_for(span) == std::future_status::ready) {
                auto tile = tileFuture.get();
                // possible a tile is deleted by the main thread before it gets finished
                if(tile) {
                    const TileID& id = tile->getID();
                    logMsg("Tile [%d, %d, %d] finished loading\n", id.z, id.x, id.y);
                    m_tileSet[id] = tile;
                    // tile is now loaded, removed its proxies
                    cleanProxyTiles(tile->getID());
                    tileSetChanged = true;
                    incomingTilesIter = m_incomingTiles.erase(incomingTilesIter);
                }
            } else {
                incomingTilesIter++;
            }

        }
    }

    if (! (m_view->changedSinceLastCheck() || tileSetChanged) ) {
        // No new tiles have come into view and no tiles have finished loading, 
        // so the tileset is unchanged
        return false;
    }

    const std::set<TileID>& visibleTiles = m_view->getVisibleTiles();

    auto tileSetIter = m_tileSet.begin();
    auto visTilesIter = visibleTiles.begin();

    while (tileSetIter != m_tileSet.end() && visTilesIter != visibleTiles.end()) {

        TileID visTile = *visTilesIter;

        TileID tileInSet = tileSetIter->first;

        if (visTile == tileInSet) {
            // Tiles match here, nothing to do
            // visible tile is already in m_tileSet
            // Possible a proxy tile in m_tileSet is visible now (set status to true and resetProxyCounter)
            tileSetIter->second->resetProxyCounter();
            tileSetIter->second->setState(true);
            visTilesIter++;
            tileSetIter++;
        } else if (visTile < tileInSet) {
            // tileSet is missing an element present in visibleTiles
            // A new tile became visible
            // fetch/add the tile
            addTile(visTile, m_view->isZoomIn());
            tileSetChanged = true;
            visTilesIter++;
        } else {
            // visibleTiles is missing an element present in tileSet
            // logically deletion of tile
            tileSetIter->second->setState(false);
            tileSetIter++;
            tileSetChanged = true;
        }

    }

    while (tileSetIter != m_tileSet.end()) {
        // All tiles in tileSet that haven't been covered yet are not in visibleTiles, so remove them
        // logical deletion of tiles
        tileSetIter->second->setState(false);
        tileSetIter++;
        tileSetChanged = true;
    }

    while (visTilesIter != visibleTiles.end()) {
        // All tiles in visibleTiles that haven't been covered yet are not in tileSet, so add them
        // New tile(s) are now visible
        // fetch/add the tile(s)
        addTile(*visTilesIter, m_view->isZoomIn());
        tileSetChanged = true;
        visTilesIter++;
    }
    
    // clean m_tileSet
    tileSetIter = m_tileSet.begin();
    while(tileSetIter != m_tileSet.end()) {
        if(!tileSetIter->second->getState() && tileSetIter->second->getProxyCounter() == 0) {
            removeTile(tileSetIter);
            continue;
        }
        tileSetIter++;
    }

    return tileSetChanged;
}

inline void TileManager::makeTile(std::shared_ptr<MapTile>& _mapTile, const std::unique_ptr<DataSource>& _dataSource) {
    logMsg("Loading tile [%d, %d, %d]\n", _mapTile->getID().z, _mapTile->getID().x, _mapTile->getID().y);
    if ( ! _dataSource->loadTileData(*_mapTile)) {
        logMsg("ERROR: Loading failed for tile [%d, %d, %d]\n", _mapTile->getID().z, _mapTile->getID().x, _mapTile->getID().y);
    }
        
    auto tileData = _dataSource->getTileData(_mapTile->getID());
        
    for (auto& style : m_scene->getStyles()) {
        if(tileData)
            style->addData(*tileData, *_mapTile, m_view->getMapProjection());
    }
}

inline void TileManager::updateProxyTiles(const TileID& _tileID, bool _zoomStatus) {
    if(!_zoomStatus) {
        //zoom in - add children
        std::vector<TileID> children;
        _tileID.getChildren(children, m_view->s_maxZoom);
        
        for(auto& proxyID : children) {
            // only set a proxyTile if it has its vbos ready
            if(m_tileSet.find(proxyID) != m_tileSet.end() && m_tileSet[proxyID]->hasGeometry()) {
                m_tileSet[proxyID]->incProxyCounter();
            }
        }
    }
    else {
        // zoom out - add parent
        TileID* parent = _tileID.getParent();
        
        if(parent) {
            if(m_tileSet.find(*parent) != m_tileSet.end() && m_tileSet[*parent]->hasGeometry()) {
                m_tileSet[*parent]->incProxyCounter();
            }
        }
    }
}

void TileManager::addTile(const TileID& _tileID, bool _zoomState) {
    
    std::shared_ptr<MapTile> tile(new MapTile(_tileID, m_view->getMapProjection()));
    m_tileSet[_tileID] = tile;
    m_tileSet[_tileID]->setState(true);
    
    //Add Proxy if corresponding proxy MapTile ready
    updateProxyTiles(_tileID, _zoomState);

    std::future< std::shared_ptr<MapTile> > incoming = std::async(std::launch::async, [&](TileID _id) {
        
        // Check if tile to be loaded is still required! (either not culled from m_tileSet and not logically deleted)
        // if not set the shared state of this async's future to "null"
        if(m_tileSet.find(_id) != m_tileSet.end() && m_tileSet[_id]->getState()) {
            auto threadTile = m_tileSet[_id];
            
            // Now Start fetching new tile
            for(const auto& dataSource : m_dataSources) {
                makeTile(threadTile, dataSource);
            }
            
            return threadTile;
        }
        else {
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

void TileManager::removeTile(const TileID& _tileID) {
    m_tileSet.erase(_tileID);
}

void TileManager::cleanProxyTiles(const TileID& _tileID) {
    // check if parent proxy is present
    TileID *parent = _tileID.getParent();
    
    if(parent) {
        if(m_tileSet.find(*parent) != m_tileSet.end()) {
            m_tileSet[*parent]->decProxyCounter();
        }
    }
    
    // check if any child proxies are present
    std::vector<TileID> children;
    _tileID.getChildren(children, m_view->s_maxZoom);
    for(auto& child : children) {
        if(m_tileSet.find(child) != m_tileSet.end()) {
            m_tileSet[child]->decProxyCounter();
        }
    }
    
}
