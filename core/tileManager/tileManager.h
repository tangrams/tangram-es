#pragma once

#include <vector>
#include <map>

#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "dataSource/dataSource.h"
#include "mapTile/mapTile.h"
#include "viewModule/viewModule.h"

/*
 * TileManager - A singleton class that functions as an accountant of MapTiles
 */
class TileManager {

public:
    
    //C++11 thread-safe implementation for a singleton
    static TileManager&& GetInstance() {
        static TileManager *instance = new TileManager();
        return std::move(*instance);
    }

    // move constructor required to:
    // 1. disable copy constructor
    // 2. disable assignment operator
    // 3. because tileManager is singleton
    // 4. required for some smarts done with std::move
    TileManager(TileManager&& _other);

    virtual ~TileManager();

    bool updateTileSet(); //contacts the view Module to see if tiles need updating

    std::vector<MapTile*> getVisibleTiles();

    void setView(std::shared_ptr<ViewModule> _view);
    
    void addDataSource(DataSource* _source);

    std::vector<std::unique_ptr<DataSource>>&& GetDataSources();

private:

    TileManager();

    std::shared_ptr<ViewModule> m_viewModule;

    // TODO: std::map is probably overkill, we just need a set of MapTiles ordered by tileIDs
    std::map<glm::ivec3, std::unique_ptr<MapTile>, MapTile::tileIDComparator> m_tileSet; 

    std::vector<std::unique_ptr<DataSource>> m_dataSources;

    std::vector<glm::ivec3> m_tilesToAdd;

    void addTile(const glm::ivec3& _tileID);
    void removeTile(const decltype(m_tileSet)::iterator& _tileIter);

};
