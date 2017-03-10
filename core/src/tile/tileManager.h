#pragma once

#include "data/tileData.h"
#include "data/tileSource.h"
#include "tile/tile.h"
#include "tile/tileID.h"
#include "tile/tileTask.h"
#include "tile/tileWorker.h"
#include "util/fastmap.h"

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
struct ViewState;

/* Singleton container of <TileSet>s
 *
 * TileManager is a singleton that maintains a set of Tiles based on the current
 * view into the map
 */
class TileManager {

    const static size_t DEFAULT_CACHE_SIZE = 32*1024*1024; // 32 MB

public:

    TileManager(std::shared_ptr<Platform> platform, TileTaskQueue& _tileWorker);

    virtual ~TileManager();

    /* Sets the tile TileSources */
    void setTileSources(const std::vector<std::shared_ptr<TileSource>>& _sources);

    /* Updates visible tile set and load missing tiles */
    void updateTileSets(const ViewState& _view, const std::set<TileID>& _visibleTiles);

    void clearTileSets();

    void clearTileSet(int32_t _sourceId);

    /* Returns the set of currently visible tiles */
    const auto& getVisibleTiles() const { return m_tiles; }

    bool hasTileSetChanged() { return m_tileSetChanged; }

    bool hasLoadingTiles() {
        return m_tilesInProgress > 0;
    }

    void addClientTileSource(std::shared_ptr<TileSource> _source);

    bool removeClientTileSource(TileSource& _source);

    std::unique_ptr<TileCache>& getTileCache() { return m_tileCache; }

    const auto& getTileSets() { return m_tileSets; }

    /* @_cacheSize: Set size of in-memory tile cache in bytes.
     * This cache holds recently used <Tile>s that are ready for rendering.
     */
    void setCacheSize(size_t _cacheSize);

private:

    enum class ProxyID : uint8_t {
        no_proxies = 0,
        child1 = 1 << 0,
        child2 = 1 << 1,
        child3 = 1 << 2,
        child4 = 1 << 3,
        parent = 1 << 4,
        parent2 = 1 << 5,
    };

    struct TileEntry {

        TileEntry(){}
        TileEntry(std::shared_ptr<Tile>& _tile) : tile(_tile) {}

        ~TileEntry() { clearTask(); }

        std::shared_ptr<Tile> tile;
        std::shared_ptr<TileTask> task;

        /* A Counter for number of tiles this tile acts a proxy for */
        int m_proxyCounter = 0;

        /* The set of proxy tiles referenced by this tile */
        uint8_t m_proxies = 0;

        bool isReady() { return bool(tile); }
        bool isInProgress() { return bool(task) && !task->isCanceled(); }

        bool needsLoading() {
            //return !bool(task) || (task->needsLoading() && !task->isCanceled());
            if (isReady()) { return false; }
            if (!task) { return true; }
            if (task->isCanceled()) { return false; }
            if (task->needsLoading()) { return true; }

            for (auto& subtask : task->subTasks()) {
                if (subtask->needsLoading()) { return true; }
            }
            return false;
        }

        // size_t rastersPending() {
        //     if (task) {
        //         return (task->source().rasterSources().size() - task->subTasks().size());
        //     }
        //     return 0;
        // }
        bool isCanceled() { return bool(task) && task->isCanceled(); }

        // New Data only when
        // - task still exists
        // - task has a tile ready
        // - tile has all rasters set
        bool newData() {
            if (bool(task) && task->isReady()) {

                //if (rastersPending()) { return false; }

                for (auto& rTask : task->subTasks()) {
                    if (!rTask->isReady()) { return false; }
                }
                return true;
            }
            return false;
        }

        void clearTask() {
            if (task) {
                for (auto& raster : task->subTasks()) {
                    raster->cancel();
                }
                task->subTasks().clear();
                task->cancel();

                task.reset();
            }
        }

        /*
         * Methods to set and get proxy counter
         */
        int getProxyCounter() { return m_proxyCounter; }
        void incProxyCounter() { m_proxyCounter++; }
        void decProxyCounter() { m_proxyCounter = m_proxyCounter > 0 ? m_proxyCounter - 1 : 0; }
        void resetProxyCounter() { m_proxyCounter = 0; }

        bool setProxy(ProxyID id) {
            if ((m_proxies & static_cast<uint8_t>(id)) == 0) {
                m_proxies |= static_cast<uint8_t>(id);
                return true;
            }
            return false;
        }

        bool unsetProxy(ProxyID id) {
            if ((m_proxies & static_cast<uint8_t>(id)) != 0) {
                m_proxies &= ~static_cast<uint8_t>(id);
                return true;
            }
            return false;
        }

        bool m_visible = false;

        /* Method to check whther this tile is in the current set of visible tiles
         * determined by view::updateTiles().
         */
        bool isVisible() const {
            return m_visible;
        }

        void setVisible(bool _visible) {
            m_visible = _visible;
        }
    };

    struct TileSet {
        TileSet(std::shared_ptr<TileSource> _source, bool _clientSource)
            : source(_source), clientTileSource(_clientSource) {}

        std::shared_ptr<TileSource> source;
        std::map<TileID, TileEntry> tiles;
        int64_t sourceGeneration = 0;
        bool clientTileSource;
    };

    void updateTileSet(TileSet& tileSet, const ViewState& _view, const std::set<TileID>& _visibleTiles);

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
