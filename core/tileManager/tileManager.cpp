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

bool TileManager::updateTileSet(const std::vector<Style>& _styles) {
    
    if (!(m_viewModule->viewChanged())) {
        return false;
    }

    std::vector<glm::ivec3> tilesToFetch;
    bool tileSetChanged = false;

    const std::set<glm::ivec3>& visibleTiles = m_viewModule->getVisibleTiles();

    auto tileSetIter = m_tileSet.begin();
    auto visTilesIter = visibleTiles.begin();

    while (tileSetIter != m_tileSet.end() && visTilesIter != visibleTiles.end()) {

        glm::ivec3 visTile = *visTilesIter;

        glm::ivec3 tileInSet = *(tileSetIter->first);

        if (visTile == tileInSet) {
            // Tiles match here, nothing to do
            visTilesIter++;
            tileSetIter++;
        } else if (MapTile::tileIDComparator(visTile, tileInSet)) {
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

    // For now, synchronously load the tiles we need
    if (m_tilesToAdd.size() > 0) {
        for (auto source& : m_dataSources) {
            source->LoadTile(tilesToAdd);
        }
        // Construct tiles... buckle up, this gets deep
        for (auto style& : _styles) {
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

void TileManager::setView(std::shared_ptr<ViewModule> _view) {
    m_viewModule = _view;
}

// Should be used when adding new data sources.... FUTURE
void TileManager::addDataSource(DataSource* _dataSource) {
    m_DataSources.push_back(std::unique_ptr(_dataSource));
}

std::vector<MapTile*> TileManager::getVisibleTiles() {
    return m_VisibleTiles;
}

std::vector<std::unique_ptr<DataSource>>&& TileManager::GetDataSources() {
    return std::move(m_DataSources);
}

void TileManager::addTile(const glm::ivec3& _tileID) {
    // Queue tile for loading and constructing in updateTileSet
    m_tilesToAdd.push_back(_tileID);
}

void TileManager::removeTile(const decltype(m_tileSet)::iterator& _tileIter) {
    // Remove tile from tileSet and destruct tile
    m_tileSet.erase(_tileIter);
}
