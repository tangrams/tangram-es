#include "tileManager.h"

TileManager::TileManager() {
}

TileManager::TileManager(TileManager&& _other) :
    m_tileSet(std::move(_other.m_tileSet)),
    m_dataSources(std::move(_other.m_dataSources)),
    m_viewModule(std::move(_other.m_viewModule)) {
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

        TileID tileInSet = *(tileSetIter->first);

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

    while (visTilesIter != m_VisibleTiles.end()) {
        // All tiles in visibleTiles that haven't been covered yet are not in tileSet, so add them
        addTile(*visTilesIter);
        tileSetChanged = true;
    }

    const std::vector<Style>& styles = m_sceneDefinition->getStyles();

    // For now, synchronously load the tiles we need
    if (m_tilesToAdd.size() > 0) {
        for (auto source& : m_dataSources) {
            source->LoadTile(tilesToAdd);
        }
        // Construct tiles... buckle up, this gets deep
        for (auto style& : styles) {
            for (auto tileID& : tilesToAdd) {
                for (auto source& : m_dataSources) {
                    // Instantiate a maptile
                    MapTile* tile = new MapTile(tileID, m_viewModule);
                    // Get the previously fetched tile data
                    std::shared_ptr<Json::Value> json = source.GetData(tileID);
                    // Add styled geometry to the new tile
                    style.addData(json, tile);
                    // Add the tile to our tileset
                    m_tileSet[tileID] = std::move(*tile);
                }
            }
        }
        m_tilesToAdd.clear();
    }

    return tileSetChanged;
}

void TileManager::addTile(const TileID& _tileID) {
    // Queue tile for loading and constructing in updateTileSet
    m_tilesToAdd.push_back(_tileID);
}

void TileManager::removeTile(const std::map<TileID, std::unique_ptr<MapTile>>::iterator& _tileIter) {
    // Remove tile from tileSet and destruct tile
    m_tileSet.erase(_tileIter);
}
