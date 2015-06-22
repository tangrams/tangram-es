#pragma once

#include <map>
#include <list>
#include <vector>
#include <memory>
#include <future>
#include <set>
#include <mutex>

#include "tileWorker.h"
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
    void updateTileSet();

    void addToWorkerQueue(std::vector<char>&& _rawData, const TileID& _id, DataSource* _source);

    void addToWorkerQueue(std::shared_ptr<TileData>& _parsedData, const TileID& _id, DataSource* _source);
    
    /* Returns the set of currently visible tiles */
    const std::map<TileID, std::shared_ptr<MapTile>>& getVisibleTiles() { return m_tileSet; }
    
    bool hasTileSetChanged() { return m_tileSetChanged; }
    
private:

    TileManager();

    std::shared_ptr<View> m_view;
    std::shared_ptr<Scene> m_scene;
    
    std::mutex m_queueTileMutex;
    
    // TODO: Might get away with using a vector of pairs here (and for searching using std:search (binary search))
    std::map<TileID, std::shared_ptr<MapTile>> m_tileSet;
    
    std::vector<std::unique_ptr<DataSource>> m_dataSources;

    const static size_t MAX_WORKERS = 4;
    std::list<std::unique_ptr<TileWorker> > m_workers;
    
    std::list<std::unique_ptr<TileTask> > m_queuedTiles;
    std::list<TileID> m_loadQueue;
    
    bool m_tileSetChanged = false;
    
    /*
     * Constructs a future (async) to load data of a new visible tile
     *      this is also responsible for loading proxy tiles for the newly visible tiles
     * @_tileID: TileID for which new MapTile needs to be constructed
     */
    void addTile(const TileID& _tileID);
    
    /*
     * Removes a tile from m_tileSet
     */
    void removeTile(std::map<TileID, std::shared_ptr<MapTile>>::iterator& _tileIter);
    
    /*
     * Checks and updates m_tileSet with proxy tiles for every new visible tile
     *  @_tileID: TileID of the new visible tile for which proxies needs to be added
     */
    void updateProxyTiles(const TileID& _tileID);
    
    /*
     *  Once a visible tile finishes loaded and is added to m_tileSet, all its proxy(ies) MapTiles are removed
     */
    void cleanProxyTiles(const TileID& _tileID);

};
