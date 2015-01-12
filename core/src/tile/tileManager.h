#pragma once

#include <map>
#include <vector>
#include <memory>
#include <future>

#include "util/tileID.h"
#include "data/dataSource.h"

class Scene;
class MapTile;
class View;

/* Singleton container of <MapTile>s
 *
 * TileManager is a singleton that maintains a set of MapTiles based on the current view into the map
 */
class TileManager {

public:
    
    /* Returns the single instance of the TileManager */
    static std::unique_ptr<TileManager> GetInstance() {
        static std::unique_ptr<TileManager> instance (new TileManager());
        return std::move(instance);
    }

    /* Constructs a TileManager using move semantics */
    TileManager(TileManager&& _other);

    virtual ~TileManager();

    /* Sets the view for which the TileManager will maintain tiles */
    void setView(std::shared_ptr<View> _view) { m_view = _view; }

    /* Sets the scene which the TileManager will use to style tiles */
    void setScene(std::shared_ptr<Scene> _scene) { m_scene = _scene; }

    /* Adds a <DataSource> from which tile data should be retrieved */
    void addDataSource(std::unique_ptr<DataSource> _source) { m_dataSources.push_back(std::move(_source)); }

    /* Updates visible tile set if necessary
     * 
     * Contacts the <ViewModule> to determine whether the set of visible tiles has changed; if so,
     * constructs or disposes tiles as needed and returns true
     */
    bool updateTileSet();

    /* Returns the set of currently visible tiles */
    const std::map<TileID, std::unique_ptr<MapTile>>& getVisibleTiles() { return m_tileSet; }

private:

    TileManager();

    std::shared_ptr<View> m_view;
    std::shared_ptr<Scene> m_scene;

    std::map<TileID, std::unique_ptr<MapTile>> m_tileSet;

    std::vector<std::unique_ptr<DataSource>> m_dataSources;

    std::vector< std::future<MapTile*> > m_incomingTiles;

    void addTile(const TileID& _tileID);
    void removeTile(std::map<TileID, std::unique_ptr<MapTile>>::iterator& _tileIter);

};
