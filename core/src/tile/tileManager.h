#pragma once

#include "data/tileData.h"
#include "data/tileSource.h"
#include "tile/tile.h"
#include "tile/tileID.h"
#include "tile/tileTask.h"
#include "tile/tileWorker.h"

#include <map>
#include <memory>
#include <mutex>
#include <tuple>
#include <set>
#include <vector>

class Platform;

namespace Tangram {

class TileSource;
class TileCache;
class View;
struct ViewState;

/* Singleton container of <TileSet>s
 *
 * TileManager is a singleton that maintains a set of Tiles based on the current
 * view into the map
 */
class TileManager {

    const static size_t DEFAULT_CACHE_SIZE = 32*1024*1024; // 32 MB

public:

    TileManager(Platform& platform, TileTaskQueue& _tileWorker);

    virtual ~TileManager();

    /* Sets the tile TileSources */
    void setTileSources(const std::vector<std::shared_ptr<TileSource>>& _sources);

    /* Updates visible tile set and load missing tiles */
    void updateTileSets(const View& _view);

    void clearTileSets(bool clearSourceCaches = false);

    void clearTileSet(int32_t _sourceId);

    void cancelTileTasks();

    /* Returns the set of currently visible tiles */
    const auto& getVisibleTiles() const { return m_tiles; }

    bool hasTileSetChanged() { return m_tileSetChanged; }

    bool hasLoadingTiles() {
        return m_tilesInProgress > 0;
    }

    std::shared_ptr<TileSource> getClientTileSource(int32_t sourceID);

    void addClientTileSource(std::shared_ptr<TileSource> _source);

    bool removeClientTileSource(TileSource& _source);

    const std::unique_ptr<TileCache>& getTileCache() const { return m_tileCache; }

    /* @_cacheSize: Set size of in-memory tile cache in bytes.
     * This cache holds recently used <Tile>s that are ready for rendering.
     */
    void setCacheSize(size_t _cacheSize);

protected:

    enum class ProxyID : uint8_t;
    struct TileEntry;

    struct TileSet {
        TileSet(std::shared_ptr<TileSource> _source, bool _clientSource);
        ~TileSet();
        void cancelTasks();

        std::shared_ptr<TileSource> source;

        std::set<TileID> visibleTiles;
        std::map<TileID, TileEntry> tiles;

        int64_t sourceGeneration = 0;
        bool clientTileSource;
    };

    void updateTileSet(TileSet& tileSet, const ViewState& _view);

    void enqueueTask(TileSet& _tileSet, const TileID& _tileID, const ViewState& _view);

    void loadTiles();

    /*
     * Constructs a future (async) to load data of a new visible tile this is
     *      also responsible for loading proxy tiles for the newly visible tiles
     * @_tileID: TileID for which new Tile needs to be constructed
     */
    bool addTile(TileSet& _tileSet, const TileID& _tileID);

    /*
     * Removes a tile from m_tileSet
     */
    void removeTile(TileSet& _tileSet, std::map<TileID, TileEntry>::iterator& _tileIter);

    /*
     * Checks and updates m_tileSet with proxy tiles for every new visible tile
     *  @_tile: Tile, the new visible tile for which proxies needs to be added
     */
    bool updateProxyTile(TileSet& _tileSet, TileEntry& _tile, const TileID& _proxy, const ProxyID _proxyID);
    void updateProxyTiles(TileSet& _tileSet, const TileID& _tileID, TileEntry& _tile);

    /*
     * Once a visible tile finishes loading and is added to m_tileSet, all
     * its proxy(ies) Tiles are removed
     */
    void clearProxyTiles(TileSet& _tileSet, const TileID& _tileID, TileEntry& _tile, std::vector<TileID>& _removes);

    int32_t m_tilesInProgress = 0;

    std::vector<TileSet> m_tileSets;

    /* Current tiles ready for rendering */
    std::vector<std::shared_ptr<Tile>> m_tiles;

    std::unique_ptr<TileCache> m_tileCache;

    TileTaskQueue& m_workers;

    bool m_tileSetChanged = false;

    /* Callback for TileSource:
     * Passes TileTask back with data for further processing by <TileWorker>s
     */
    TileTaskCb m_dataCallback;

    /* Temporary list of tiles that need to be loaded */
    std::vector<std::tuple<double, TileSet*, TileID>> m_loadTasks;

};

}
