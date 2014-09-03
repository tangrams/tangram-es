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
    /*  --UpdateTiles--
    updateTiles contacts the DataSource for new tile data.
    Naive Implementation: Current implementation, sets the tileIDs explicitly
    Smart Todo: viewModule updates the visibleTileIDs which is then passed to the dataSource.
    Smart Todo 2: Be smart of getting new tiles :D.
    */
    void UpdateTiles();
    /*  --CalculateVisibleTileIDs--
    Fills the m_VisibleTileIDs, which will be calculated based on
    the inputs from the view module.
    */
    void CalculateVisibleTileIDs();
    TileManager();

    std::vector<MapTile*> m_VisibleTiles;
    std::vector<std::unique_ptr<DataSource>> m_DataSources;
    ViewModule *m_viewModule;
    std::vector<glm::vec3> m_VisibleTileIDs;

public:
    //C++11 thread-safe implementation for a singleton
    static TileManager&& GetInstance() {
        static TileManager *instance = new TileManager();
        return std::move(*instance);
    }

    bool CheckNewTileStatus(); //contacts the view Module to see if tiles need updating
    void AddDataSource(std::unique_ptr<DataSource>);
    std::vector<MapTile*> GetVisibleTiles();
    std::vector<std::unique_ptr<DataSource>>&& GetDataSources();

    // move constructor required to:
    // 1. disable copy constructor
    // 2. disable assignment operator
    // 3. because tileManager is singleton
    // 4. required for some smarts done with std::move
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
