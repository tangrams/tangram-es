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
    TileManager();
public:

    std::vector<MapTile*> m_VisibleTiles;
    std::vector<std::unique_ptr<DataSource>> m_DataSources;
    ViewModule *m_viewModule;
    std::vector<glm::vec3> m_VisibleTileIDs;


    //C++11 thread-safe implementation for a singleton
    static TileManager&& GetInstance() {
        static TileManager *instance = new TileManager();
        return std::move(*instance);
    }

    bool CheckNewTileStatus(); //contacts the view Module to see if tiles need updating
    void AddDataSource(std::unique_ptr<DataSource>);
    std::vector<MapTile*> GetVisibleTiles();
    std::vector<std::unique_ptr<DataSource>>&& GetDataSources();

    TileManager(TileManager&& other) :
                    m_VisibleTiles(std::move(other.m_VisibleTiles)),
                    m_DataSources(std::move(other.m_DataSources)),
                    m_viewModule(std::move(other.m_viewModule)),
                    m_VisibleTileIDs(std::move(other.m_VisibleTileIDs)) {

    }

    ~TileManager() {
        m_DataSources.clear();
        m_VisibleTileIDs.clear();
        m_VisibleTiles.clear();
    }
};

#endif