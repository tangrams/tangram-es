#pragma once

#include "data/tileData.h"
#include "tile/tileWorker.h"
#include "tile/tileID.h"
#include "tileTask.h"

#include <map>
#include <list>
#include <vector>
#include <memory>
#include <future>
#include <set>
#include <mutex>
#include <tuple>

namespace Tangram {

class DataSource;
class Tile;
class Scene;
class View;
class TileCache;
class ClientGeoJsonSource;

/* Singleton container of <Tile>s
 *
 * TileManager is a singleton that maintains a set of Tiles based on the current
 * view into the map
 */
class TileManager {

    const static size_t MAX_WORKERS = 2;
    const static size_t MAX_DOWNLOADS = 4;
    const static size_t DEFAULT_CACHE_SIZE = 32*1024*1024; // 32 MB

public:

    /* Returns the single instance of the TileManager */
    static std::unique_ptr<TileManager> GetInstance() {
        static std::unique_ptr<TileManager> instance (new TileManager());
        return std::move(instance);
    }

    virtual ~TileManager();

    /* Sets the view for which the TileManager will maintain tiles */
    void setView(std::shared_ptr<View> _view) { m_view = _view; }

    /* Sets the scene which the TileManager will use to style tiles */
    void setScene(std::shared_ptr<Scene> _scene);

    std::shared_ptr<Scene>& getScene() { return m_scene; }

    /* Updates visible tile set if necessary
     *
     * Contacts the <ViewModule> to determine whether the set of visible tiles
     * has changed; if so, constructs or disposes tiles as needed and returns
     * true
     */
    void updateTileSets();

    void clearTileSets();

    void clearTileSet(int32_t _id);

    /* For TileWorker: Pass TileTask with processed data back
     * to TileManager.
     */
    void tileProcessed(std::shared_ptr<TileTask>&& task);

    /* Returns the set of currently visible tiles */
    const auto& getVisibleTiles() { return m_tiles; }

    bool hasTileSetChanged() { return m_tileSetChanged; }

    void addDataSource(std::shared_ptr<DataSource> dataSource);

    const auto getTileSets() { return m_tileSets; }

    std::shared_ptr<ClientGeoJsonSource> getClientSourceById(int32_t _id);

    /* @_cacheSize: Set size of in-memory tile cache in bytes.
     * This cache holds recently used <Tile>s that are ready for rendering.
     */
    void setCacheSize(size_t _cacheSize);

private:

    struct TileSet {
        int32_t id;
        std::shared_ptr<DataSource> source;
        std::map<TileID, std::shared_ptr<Tile>> tiles;
    };

    TileManager();

    void updateTileSet(TileSet& tileSet);

    bool setTileState(Tile& tile, TileState state);

    void enqueueTask(const TileSet& tileSet, const TileID& tileID, const glm::dvec2& viewCenter);

    void loadTiles();

    /*
     * Constructs a future (async) to load data of a new visible tile this is
     *      also responsible for loading proxy tiles for the newly visible tiles
     * @_tileID: TileID for which new Tile needs to be constructed
     */
    bool addTile(TileSet& tileSet, const TileID& _tileID);

    /*
     * Removes a tile from m_tileSet
     */
    void removeTile(TileSet& tileSet, std::map<TileID, std::shared_ptr<Tile>>::iterator& _tileIter,
                    std::vector<TileID>& _removes);

    /*
     * Checks and updates m_tileSet with proxy tiles for every new visible tile
     *  @_tile: Tile, the new visible tile for which proxies needs to be added
     */
    void updateProxyTiles(TileSet& tileSet, Tile& _tile);

    /*
     * Once a visible tile finishes loading and is added to m_tileSet, all
     * its proxy(ies) Tiles are removed
     */
    void clearProxyTiles(TileSet& tileSet, Tile& _tile, std::vector<TileID>& _removes);

    std::shared_ptr<View> m_view;
    std::shared_ptr<Scene> m_scene;

    std::mutex m_readyTileMutex;
    std::mutex m_tileStateMutex;
    int32_t m_loadPending;

    std::vector<TileSet> m_tileSets;

    /* Current tiles ready for rendering */
    std::vector<std::shared_ptr<Tile>> m_tiles;

    std::unique_ptr<TileCache> m_tileCache;

    std::unique_ptr<TileWorker> m_workers;

    bool m_tileSetChanged = false;

    /* Callback for DataSource:
     * Passes TileTask back with data for further processing by <TileWorker>s
     */
    TileTaskCb m_dataCallback;

    /* Temporary list of tiles that need to be loaded */
    std::vector<std::tuple<double, const TileSet*, const TileID*>> m_loadTasks;

    /* List of tiles passed from <TileWorker> threads */
    std::vector<std::shared_ptr<TileTask>> m_readyTiles;
};

}
