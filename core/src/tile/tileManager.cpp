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
            
            std::future<MapTile*>& tileFuture = *incomingTilesIter;
            std::chrono::milliseconds span (0);
            
            if (tileFuture.wait_for(span) == std::future_status::ready) {
                MapTile* tile = tileFuture.get();
                const TileID& id = tile->getID();
                logMsg("Tile [%d, %d, %d] finished loading\n", id.z, id.x, id.y);
                m_tileSet[id].reset(tile);
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
            addTile(visTile);
            tileSetChanged = true;
            visTilesIter++;
        } else {
            // visibleTiles is missing an element present in tileSet
            removeTile(tileSetIter);
            tileSetChanged = true;
        }

    }

    while (tileSetIter != m_tileSet.end()) {
        // All tiles in tileSet that haven't been covered yet are not in visibleTiles, so remove them
        removeTile(tileSetIter);
        tileSetChanged = true;
    }

    while (visTilesIter != visibleTiles.end()) {
        // All tiles in visibleTiles that haven't been covered yet are not in tileSet, so add them
        addTile(*visTilesIter);
        visTilesIter++;
        tileSetChanged = true;
    }

    return tileSetChanged;
}

void TileManager::addTile(const TileID& _tileID) {

    m_tileSet[_tileID].reset(nullptr); // Emplace a unique_ptr that is null until tile finishes loading

    std::future<MapTile*> incoming = std::async(std::launch::async, [&](TileID _id) {

        MapTile* tile = new MapTile(_id, m_view->getMapProjection());

        for (const auto& source : m_dataSources) {
            
            logMsg("Loading tile [%d, %d, %d]\n", _id.z, _id.x, _id.y);
            if ( ! source->loadTileData(*tile)) {
                logMsg("ERROR: Loading failed for tile [%d, %d, %d]\n", _id.z, _id.x, _id.y);
            }
            
            std::shared_ptr<TileData> tileData = source->getTileData(_id);
            
            for (auto& style : m_scene->getStyles()) {
                style->addData(*tileData, *tile, m_view->getMapProjection());
            }
            
        }

        return tile;

    }, _tileID);
    
    m_incomingTiles.push_back(std::move(incoming));
    
}

void TileManager::removeTile(std::map< TileID, std::unique_ptr<MapTile> >::iterator& _tileIter) {
    
    // Remove tile from tileSet and destruct tile
    _tileIter = m_tileSet.erase(_tileIter);

    // TODO: if tile is being loaded, cancel loading; For now they continue to load
    // and will be culled the next time that updateTileSet is called
    
}
