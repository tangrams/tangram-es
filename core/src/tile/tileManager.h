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

namespace Tangram {

class DataSource;
class Tile;
class Scene;
class View;
class TileCache;

/* Singleton container of <Tile>s
 *
 * TileManager is a singleton that maintains a set of Tiles based on the current view into the map
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
    void setScene(std::shared_ptr<Scene> _scene) { m_scene = _scene; }

    std::shared_ptr<Scene>& getScene() { return m_scene; }

    /* Updates visible tile set if necessary
     *
     * Contacts the <ViewModule> to determine whether the set of visible tiles has changed; if so,
     * constructs or disposes tiles as needed and returns true
     */
    void updateTileSet();

    void clearTileSet();

    /* For TileWorker: Pass TileTask with processed data back
     * to TileManager.
     */
    void tileProcessed(std::shared_ptr<TileTask>&& task);

    /* Returns the set of currently visible tiles */
    const auto& getVisibleTiles() { return m_tileSet; }

    auto& dataSources() { return m_dataSources; }

    bool hasTileSetChanged() { return m_tileSetChanged; }

private:

    TileManager();

    std::shared_ptr<View> m_view;
    std::shared_ptr<Scene> m_scene;

    std::mutex m_readyTileMutex;
    std::vector<std::shared_ptr<TileTask>> m_readyTiles;

    std::mutex m_tileStateMutex;
    int32_t m_loadPending;

    // TODO: Might get away with using a vector of pairs here (and for searching
    // using std:search (binary search))
    std::map<TileID, std::shared_ptr<Tile>> m_tileSet;

    std::vector<std::shared_ptr<DataSource>> m_dataSources;

    std::unique_ptr<TileCache> m_tileCache;

    std::unique_ptr<TileWorker> m_workers;

    bool m_tileSetChanged = false;

    /* For DataSource: Pass TileTask with 'parsed' or 'raw' data back
     * to TileManager for further processing by TileWorker.
     */
    TileTaskCb m_dataCallback;

    std::vector<std::pair<double, const TileID*>> m_loadTasks;

    /*
     * Constructs a future (async) to load data of a new visible tile
     *      this is also responsible for loading proxy tiles for the newly visible tiles
     * @_tileID: TileID for which new Tile needs to be constructed
     */
    bool addTile(const TileID& _tileID);

    /*
     * Removes a tile from m_tileSet
     */
    void removeTile(std::map<TileID, std::shared_ptr<Tile>>::iterator& _tileIter,
                    std::vector<TileID>& _removes);

    /*
     * Checks and updates m_tileSet with proxy tiles for every new visible tile
     *  @_tile: Tile, the new visible tile for which proxies needs to be added
     */
    void updateProxyTiles(Tile& _tile);

    /*
     *  Once a visible tile finishes loading and is added to m_tileSet, all its proxy(ies) Tiles are removed
     */
    void clearProxyTiles(Tile& _tile, std::vector<TileID>& _removes);

    bool setTileState(Tile& tile, TileState state);

    void enqueueLoadTask(const TileID& tileID, const glm::dvec2& viewCenter);

};

}
