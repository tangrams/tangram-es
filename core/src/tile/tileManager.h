#pragma once

#include <map>
#include <vector>
#include <memory>
#include <future>
#include <set>
#include <mutex>

#include "glm/glm.hpp"

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
    const std::map<TileID, std::shared_ptr<MapTile>>& getVisibleTiles() { return m_tileSet; }
    
    /* Returns the set of proxy tiles */
    const std::map<TileID, std::shared_ptr<MapTile>>& getProxyTiles() { return m_proxyTiles; }
    
private:

    TileManager();

    std::shared_ptr<View> m_view;
    std::shared_ptr<Scene> m_scene;
    
    /*
     * mutex as multiple threads could add/remove proxy tiles
     */
    std::mutex m_proxyMutex;

    // TODO: Might get away with using a vector of pairs here (and for searching using std:search (binary search))
    std::map<TileID, std::shared_ptr<MapTile>> m_tileSet;
    
    std::map<TileID, std::shared_ptr<MapTile>> m_proxyTiles;

    std::vector<std::unique_ptr<DataSource>> m_dataSources;

    std::vector< std::future<std::shared_ptr<MapTile>> > m_incomingTiles;
    
    /*
     * Checks and updates m_proxyTiles with proxy tiles for every new visible tile
     *      Any proxy tile which is all ready to be drawn is passed in m_proxyTiles serially
     *      Any proxy tile which will require tesselation is handled by the visible tile loading async thread
     *  @_tileID: TileID of the new visible tile for which proxies needs to be added
     *  @_zoomStatus: Zoom-in or Zoom-out to determine parent of child proxies
     */
    void updateProxyTiles(const TileID& _tileID, bool _zoomStatus, bool _serial=false);
    
    /*
     * Constructs a future (async) to load data of a new visible tile
     *      this is also responsible for loading proxy tiles for the newly visible tiles
     * @_tileID: TileID for which new MapTile needs to be constructed
     * @_zoomState: to determine whether to add child tiles or parent tile as proxy for this new visible tile
     */
    void addTile(const TileID& _tileID, bool _zoomState);
    
    /*
     *  Adds parent or child proxy tiles based on the zoomState (in or out).
     *  Grabs the MapTile from m_tileSet if this proxy tile was visible last update call (serially)
     *  Or, constructs a new MapTile from the prewviously fetched raw tile data (async)
     *  @ _proxyID: TileID of the proxy tile to be added to m_proxyTiles
     */
    void addProxyTile(const TileID& _proxyID, bool _serial = false);
    
    /*
     *  Overloaded removeTile functions to remove items from m_tileSet or m_proxyTiles
     */
    void removeTile(std::map<TileID, std::shared_ptr<MapTile>>::iterator& _tileIter, bool _isProxy=false);
    void removeTile(const TileID& _tileID, bool _isProxy=false);
    
    /*
     *  Sets the drawing status of the MapTile to false
     *  --- Logical Deletion---
     */
    void unsetTileDrawable(std::map<TileID, std::shared_ptr<MapTile>>::iterator& _tileIter);
    
    /*
     *  Sets the drawing status of the MapTile to true
     */
    void setTileDrawable(const TileID& _tileID, bool isProxy=false);
    
    /*
     *  Once a visible tile finishes loaded and is added to m_tileSet, all its proxy(ies) MapTiles are removed
     */
    void cleanProxyTiles(const TileID& _tileID);
    
    /*
     *  Makes a MapTile by fetching data from dataSource and constructing VboMeshes for this tile
     */
    void makeTile(std::shared_ptr<MapTile>& _mapTile, const std::unique_ptr<DataSource>& _dataSource);

};
