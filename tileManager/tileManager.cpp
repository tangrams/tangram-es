#include "tileManager.h"
#include "../dataSource/dataSource.h"
#include "../mapTile/mapTile.h"

#include <iostream>

bool TileManager::CheckNewTileStatus() {
    //Todo: Contact the view module to get new tile needed?
    // if True, call updateVisibleTiles()
    // else, nothing
    if (true) {
        //CalculateVisibleTileIDs();
        UpdateTiles();
    }
    return true;
}

void TileManager::UpdateTiles() {
    DataSource *ds = m_DataSources.at(0).get();
    m_VisibleTileIDs.push_back(glm::vec3(0,0,0));
    m_VisibleTileIDs.push_back(glm::vec3(16,19293,24641));
    m_VisibleTileIDs.push_back(glm::vec3(14,19293,24641));
    ds->LoadTile(m_VisibleTileIDs);
    for(auto tileIDItr = m_VisibleTileIDs.begin();
            tileIDItr != m_VisibleTileIDs.end();
            tileIDItr++) {
        MapTile *mapTile = new MapTile(*tileIDItr);
        m_VisibleTiles.push_back(mapTile);
    }

}

TileManager::TileManager() {
}

void TileManager::AddDataSource(std::unique_ptr<DataSource> ds) {
    m_DataSources.push_back(std::move(ds));
}

std::vector<MapTile*> TileManager::GetVisibleTiles() {
    return m_VisibleTiles;
}

std::vector<std::unique_ptr<DataSource>>&& TileManager::GetDataSources() {
    return std::move(m_DataSources);
}