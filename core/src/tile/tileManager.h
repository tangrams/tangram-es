#pragma once

#include "data/tileData.h"
#include "tile/tileWorker.h"
#include "tile/tile.h"
#include "tile/tileID.h"
#include "tileTask.h"

#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <tuple>

namespace Tangram {

class DataSource;
class Scene;
class View;
class TileCache;

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

    TileManager();

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

    void clearTileSet(int32_t _sourceId);

    /* Returns the set of currently visible tiles */
    const auto& getVisibleTiles() { return m_tiles; }

    bool hasTileSetChanged() { return m_tileSetChanged; }

    void addDataSource(std::shared_ptr<DataSource> dataSource);

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

        std::shared_ptr<Tile> tile;
        std::shared_ptr<TileTask> task;

        /* A Counter for number of tiles this tile acts a proxy for */
        int m_proxyCounter = 0;

        uint8_t m_proxies = 0;

        bool isReady() { return bool(tile); }
        bool isLoading() {
            // FIXME remove second condition (see m_dataCallback)
            return bool(task) && !task->isCanceled();
        }

        bool newData() {
            return bool(task) && bool(task->tile);
        }

        void cancelTask() {
            if (task) {
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

        bool m_visible;

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
        std::shared_ptr<DataSource> source;
        std::map<TileID, TileEntry> tiles;
        int64_t sourceGeneration;
    };

    void updateTileSet(TileSet& tileSet);

    void enqueueTask(TileSet& tileSet, const TileID& tileID, const glm::dvec2& viewCenter);

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
    void removeTile(TileSet& tileSet, std::map<TileID, TileEntry>::iterator& _tileIter);

    /*
     * Checks and updates m_tileSet with proxy tiles for every new visible tile
     *  @_tile: Tile, the new visible tile for which proxies needs to be added
     */
    bool updateProxyTile(TileSet& tileSet, TileEntry& _tile, const TileID& _proxy, const ProxyID _proxyID);
    void updateProxyTiles(TileSet& tileSet, const TileID& _tileID, TileEntry& _tile);

    /*
     * Once a visible tile finishes loading and is added to m_tileSet, all
     * its proxy(ies) Tiles are removed
     */
    void clearProxyTiles(TileSet& tileSet, const TileID& _tileID, TileEntry& _tile, std::vector<TileID>& _removes);

    std::shared_ptr<View> m_view;
    std::shared_ptr<Scene> m_scene;

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
    std::vector<std::tuple<double, TileSet*, const TileID*>> m_loadTasks;


};

}
