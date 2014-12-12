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
            
            std::future<std::shared_ptr<MapTile>>& tileFuture = *incomingTilesIter;
            std::chrono::milliseconds span (0);
            
            if (tileFuture.wait_for(span) == std::future_status::ready) {
                auto tile = tileFuture.get();
                const TileID& id = tile->getID();
                logMsg("Tile [%d, %d, %d] finished loading\n", id.z, id.x, id.y);
                m_tileSet[id] = tile;
                setTileDrawable(id);
                cleanProxyTiles(tile->getID());
                tileSetChanged = true;
                incomingTilesIter = m_incomingTiles.erase(incomingTilesIter);
            } else {
                incomingTilesIter++;
            }

        }
    }

    if (!(m_view->viewChanged()) && !tileSetChanged) {
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
            visTilesIter++;
            tileSetIter++;
        } else if (visTile < tileInSet) {
            // tileSet is missing an element present in visibleTiles
            
            // possible still in proxySet, check first
            if(m_proxyTiles.find(visTile) != m_proxyTiles.end()) {
                // m_tileSet gets a reference of this from proxy, also no need to add proxy for this tile now ... already loaded/cached
                m_tileSet[visTile] = m_proxyTiles[visTile];
                setTileDrawable(visTile);
                // no longer a proxy since its visible now, remove
                removeTile(visTile, true);
            }
            else {
                addTile(visTile, m_view->getZoomState());
            }
            tileSetChanged = true;
            visTilesIter++;
        } else {
            // visibleTiles is missing an element present in tileSet
            if(tileSetIter->second) {
                unsetTileDrawable(tileSetIter);
            }
            else {
                removeTile(tileSetIter);
                continue;
            }
            tileSetIter++;
            tileSetChanged = true;
        }

    }

    while (tileSetIter != m_tileSet.end()) {
        // All tiles in tileSet that haven't been covered yet are not in visibleTiles, so remove them
        if(tileSetIter->second) {
            unsetTileDrawable(tileSetIter);
        }
        else {
            removeTile(tileSetIter);
            continue;
        }
        tileSetIter++;
        tileSetChanged = true;
    }

    while (visTilesIter != visibleTiles.end()) {
        // All tiles in visibleTiles that haven't been covered yet are not in tileSet, so add them
        
        // possible still in proxySet, check first
        if(m_proxyTiles.find(*visTilesIter) != m_proxyTiles.end()) {
            // m_tileSet gets a reference of this from proxy, also no need to add proxy for this tile now ... already loaded/cached
            m_tileSet[*visTilesIter] = m_proxyTiles[*visTilesIter];
            setTileDrawable(*visTilesIter);
            // no longer a proxy since its visible now, remove
            removeTile(*visTilesIter, true);
        }
        else {
            addTile(*visTilesIter, m_view->getZoomState());
        }
        visTilesIter++;
        tileSetChanged = true;
    }
    
    // clean m_tileSet by syncing m_tileSet and m_proxyTileSet
    tileSetIter = m_tileSet.begin();
    while(tileSetIter != m_tileSet.end()) {
        if(tileSetIter->second) {
            // for tiles marked not to be drawn in m_tileSet
            if(! tileSetIter->second->getStatus()) {
                if(m_proxyTiles.find(tileSetIter->first) != m_proxyTiles.end()) {
                    setTileDrawable(tileSetIter->first, true);
                    removeTile(tileSetIter);
                    continue;
                }
            }
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
        
    std::shared_ptr<TileData> tileData = _dataSource->getTileData(_mapTile->getID());
        
    for (auto& style : m_scene->getStyles()) {
        if(tileData)
            style->addData(*tileData, *_mapTile, m_view->getMapProjection());
    }
}

inline void TileManager::addProxyTile(const TileID& _proxyID) {
    for(const auto& dataSource : m_dataSources) {
        // check if proxy was previously loaded
        if(dataSource->hasTileData(_proxyID)) {
            if(m_tileSet.find(_proxyID) != m_tileSet.end() && m_tileSet[_proxyID]) {
                // if tile exists in m_tileSet grab from there and has valid data (is fully loaded)
                {
                    std::lock_guard<std::mutex> lock(m_proxyMutex);
                    m_proxyTiles[_proxyID] = m_tileSet[_proxyID];
                }
            }
            else {
                // no reference of this MapTile exists
                // create MapTile from already loaded tileData
                std::shared_ptr<MapTile> proxyTile(new MapTile(_proxyID, m_view->getMapProjection()));
                makeTile(proxyTile, dataSource);
                {
                    std::lock_guard<std::mutex> lock(m_proxyMutex);
                    m_proxyTiles[_proxyID] = proxyTile;
                }
                setTileDrawable(_proxyID, true);
            }
            break;
        }
    }
}

void TileManager::updateProxyTiles(const TileID& _tileID, bool _zoomStatus) {
    if(!_zoomStatus) {
        //zoom in - add children
        std::vector<TileID> children;
        _tileID.getChildren(children, 18);
        
        for(auto& proxyID : children) {
            addProxyTile(proxyID);
        }
    }
    else {
        // zoom out - add parent
        TileID* parent = _tileID.getParent();
        
        if(parent) {
            addProxyTile(*parent);
        }
    }
}

void TileManager::addTile(const TileID& _tileID, bool _zoomState) {
    
    m_tileSet[_tileID] = nullptr; // Emplace a null until tile finishes loading
    //std::shared_ptr<MapTile> tile(new MapTile(_tileID, m_view->getMapProjection()));
    //m_tileSet[_tileID] = tile;

    std::future< std::shared_ptr<MapTile> > incoming = std::async(std::launch::async, [&](TileID _id) {
        
        // Generate proxies for this tile
        updateProxyTiles(_id, _zoomState);

        std::shared_ptr<MapTile> threadTile(new MapTile(_id, m_view->getMapProjection()));
        //std::shared_ptr<MapTile> threadTile = m_tileSet[_id];
        
        // Now Start fetching new tile
        for(const auto& dataSource : m_dataSources) {
            makeTile(threadTile, dataSource);
        }
        
        return threadTile;

    }, _tileID);
    
    m_incomingTiles.push_back(std::move(incoming));
    
}

void TileManager::removeTile(std::map< TileID, std::shared_ptr<MapTile> >::iterator& _tileIter, bool _isProxy) {
    
    if(_isProxy) {
        {
            // Remove tile from tileSet and destruct tile
            std::lock_guard<std::mutex> lock(m_proxyMutex);
            _tileIter = m_proxyTiles.erase(_tileIter);
        }
    }
    else {
        // Remove tile from tileSet and destruct tile
        _tileIter = m_tileSet.erase(_tileIter);
    }

    // TODO: if tile is being loaded, cancel loading; For now they continue to load
    // and will be culled the next time that updateTileSet is called
    
}

void TileManager::removeTile(const TileID& _tileID, bool _isProxy) {
    if(_isProxy) {
        {
            std::lock_guard<std::mutex> lock(m_proxyMutex);
            m_proxyTiles.erase(_tileID);
        }
    }
    else {
        m_tileSet.erase(_tileID);
    }
}

void TileManager::unsetTileDrawable(std::map<TileID, std::shared_ptr<MapTile>>::iterator& _tileIter) {
    _tileIter->second->setStatus(false);
}

void TileManager::setTileDrawable(const TileID& _tileID, bool _isProxy) {
    if(_isProxy) {
        m_proxyTiles[_tileID]->setStatus(true);
    }
    else {
        m_tileSet[_tileID]->setStatus(true);
    }
}

void TileManager::cleanProxyTiles(const TileID& _tileID) {
    // check if parent proxy is present
    TileID *parent = _tileID.getParent();
    if(m_proxyTiles.find(*parent) != m_proxyTiles.end()) {
        removeTile(*parent, true);
    }
    
    // check if any child proxies are present
    std::vector<TileID> children;
    _tileID.getChildren(children, 18);
    for(auto& child : children) {
        if(m_proxyTiles.find(child) != m_proxyTiles.end()) {
            removeTile(child, true);
        }
    }
    
}
