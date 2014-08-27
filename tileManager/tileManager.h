/*
...
*/

#ifndef __TILE_MANAGER_H__
#define __TILE_MANAGER_H__

#include <vector>
#include <memory>

#include "glm/glm.hpp"

//Forward Declaration
class MapTile;
class DataSource;
class ViewModule;

/* -- Todo: Singleton Class Implementation -- */
class TileManager {
    // updateTiles contacts the DataSource for new tile info.
    // naive implementation: dumps all present tiles and gets all needed
    // smart implementation: dumps not needed ones based on the tileID,
    // and gets only the ones needed for an update.
    void UpdateTiles();
    TileManager(DataSource*);

    std::vector<MapTile*> m_VisibleTiles;
    std::vector<DataSource*> m_DataSources;
    ViewModule *m_viewModule;
    std::vector<glm::vec3> m_visibleTileIDs;

public:
    //C++11 thread-safe implementation for a singleton
    static TileManager& GetInstance(std::unique_ptr<DataSource> dataSource) {
        static TileManager *instance = new TileManager(dataSource.get());
        return *instance;
    }

    bool CheckNewTileStatus(); //contacts the view Module to see if tiles need updating
    void AddDataSource(DataSource*);
    std::vector<MapTile*> GetVisibleTiles();
    std::vector<DataSource*> GetDataSources();
    ~TileManager() {
        m_VisibleTiles.clear();
        m_DataSources.clear();
        m_visibleTileIDs.clear();
    }
};

#endif