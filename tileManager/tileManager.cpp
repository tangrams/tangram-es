#include "tileManager.h"
#include "../dataSource/dataSource.h"
#include "../mapTile/mapTile.h"

bool TileManager::CheckNewTileStatus() {
    //Todo: Contact the view module to get new tile needed?
    // if True, call updateVisibleTiles()
    // else, nothing
    if (true) {
        UpdateTiles();
    }
    return true;
}

void TileManager::UpdateTiles() {
    DataSource *ds = m_DataSources.at(0);
    m_visibleTileIDs = ds->LoadGeoJsonFile();
    for(auto tileIDItr = m_visibleTileIDs.begin();
            tileIDItr != m_visibleTileIDs.end();
            tileIDItr++) {
        MapTile *mapTile = new MapTile(*tileIDItr);
        m_VisibleTiles.push_back(mapTile);
    }

}

TileManager::TileManager(DataSource *ds) {
    m_DataSources.push_back(ds);
}

void TileManager::AddDataSource(DataSource *ds) {
    m_DataSources.push_back(ds);
}

std::vector<MapTile*> TileManager::GetVisibleTiles() {
    return m_VisibleTiles;
}

std::vector<DataSource*> TileManager::GetDataSources() {
    return m_DataSources;
}