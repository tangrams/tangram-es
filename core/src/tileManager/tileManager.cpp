#include "tileManager.h"
#include "sceneDefinition/sceneDefinition.h"
#include "mapTile/mapTile.h"
#include "viewModule/viewModule.h"

TileManager::TileManager() {
}

TileManager::TileManager(TileManager&& _other) :
    m_viewModule(std::move(_other.m_viewModule)),
    m_tileSet(std::move(_other.m_tileSet)),
    m_dataSources(std::move(_other.m_dataSources)) {
}

TileManager::~TileManager() {
    m_dataSources.clear();
    m_tileSet.clear();
    m_tilesToAdd.clear();
}

bool TileManager::updateTileSet() {
    
    if (!(m_viewModule->viewChanged())) {
        return false;
    }

    bool tileSetChanged = false;

    const std::set<TileID>& visibleTiles = m_viewModule->getVisibleTiles();

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

    const std::vector<std::unique_ptr<Style>>& styles = m_sceneDefinition->getStyles();

    // For now, synchronously load the tiles we need
    if (m_tilesToAdd.size() > 0) {
        
        logMsg("Found tiles to add: \n");

        for (auto& tileID : m_tilesToAdd) {
            logMsg("    %d / %d / %d \n", tileID.z, tileID.x, tileID.y);
        }

        bool newTileLoadSuccess = false;

        for (const auto& source : m_dataSources) {
            logMsg("Loading tiles...\n");
            newTileLoadSuccess = source->LoadTile(m_tilesToAdd);
        }

        if(!newTileLoadSuccess) {
            logMsg("\n**New Tiles loading failed ... Timeout??**\n");
            return false;
        }
        // Construct tiles... buckle up, this gets deep
        for (auto& tileID : m_tilesToAdd) {
            logMsg("Building maptile %d/%d/%d:\n", tileID.z, tileID.x, tileID.y);
            // Instantiate a maptile
            std::unique_ptr<MapTile> tile(new MapTile(tileID, m_viewModule->getMapProjection()));
            for (const auto& source : m_dataSources) {
                // Get the previously fetched tile data
                std::shared_ptr<Json::Value> json = source->GetData(tileID);
                logMsg("    Retrieved JSON\n");
                if(!json) {
                    logMsg("    ***json root is null, tile was not read properly\n");
                    continue;
                }
                for (auto& style : styles) {
                    // Add styled geometry to the new tile
                    style->addData(*json, *tile, m_viewModule->getMapProjection());
                }
            }
            // Add the tile to our tileset
            m_tileSet[tileID] = std::move(tile);
        }
        m_tilesToAdd.clear();
    }

    return tileSetChanged;
}

void TileManager::addTile(const TileID& _tileID) {
    // Queue tile for loading and constructing in updateTileSet
    m_tilesToAdd.push_back(_tileID);
}

void TileManager::removeTile(std::map<TileID, std::unique_ptr<MapTile>>::iterator& _tileIter) {
    // Remove tile from tileSet and destruct tile
    m_tileSet.erase(_tileIter++);
}
