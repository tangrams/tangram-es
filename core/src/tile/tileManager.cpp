#include "tile/tileManager.h"

#include "data/tileSource.h"
#include "map.h"
#include "platform.h"
#include "tile/tile.h"
#include "tile/tileCache.h"
#include "util/mapProjection.h"
#include "view/view.h"

#include "glm/gtx/norm.hpp"

#include <algorithm>

#define DBG(...) LOG(__VA_ARGS__)

namespace Tangram {


enum class TileManager::ProxyID : uint8_t {
    no_proxies = 0,
    child1 = 1 << 0,
    child2 = 1 << 1,
    child3 = 1 << 2,
    child4 = 1 << 3,
    parent = 1 << 4,
    parent2 = 1 << 5,
};

struct TileManager::TileEntry {

    TileEntry(std::shared_ptr<Tile>& _tile)
        : tile(_tile), m_proxyCounter(0), m_proxies(0), m_visible(false) {}

    ~TileEntry() { clearTask(); }

    std::shared_ptr<Tile> tile;
    std::shared_ptr<TileTask> task;

    /* A Counter for number of tiles this tile acts a proxy for */
    int32_t m_proxyCounter;

    /* The set of proxy tiles referenced by this tile */
    uint8_t m_proxies;
    bool m_visible;

    bool isInProgress() {
        return bool(task) && !task->isCanceled();
    }

    bool isCanceled() {
        return bool(task) && task->isCanceled();
    }

    bool needsLoading() {
        if (bool(tile)) { return false; }
        if (!task) { return true; }
        if (task->isCanceled()) { return false; }
        if (task->needsLoading()) { return true; }

        for (auto& subtask : task->subTasks()) {
            if (subtask->needsLoading()) { return true; }
        }
        return false;
    }

    // Complete task only when
    // - task still exists
    // - task has a tile ready
    // - tile has all rasters set
    bool completeTileTask() {
        if (bool(task) && task->isReady()) {

            for (auto& rTask : task->subTasks()) {
                if (!rTask->isReady()) { return false; }
            }

            task->complete();
            tile = task->getTile();
            task.reset();

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

    /* Methods to set and get proxy counter */
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

TileManager::TileSet::TileSet(std::shared_ptr<TileSource> _source, bool _clientSource) :
    source(_source), clientTileSource(_clientSource) {}

TileManager::TileSet::~TileSet() {
    cancelTasks();
}

void TileManager::TileSet::cancelTasks() {
    for (auto& tile : tiles) {
        auto& entry = tile.second;
        if (entry.isInProgress()) {
            source->cancelLoadingTile(*entry.task);
        }
        entry.clearTask();
    }
}

TileManager::TileManager(Platform& platform, TileTaskQueue& _tileWorker) :
    m_workers(_tileWorker) {

    m_tileCache = std::unique_ptr<TileCache>(new TileCache(DEFAULT_CACHE_SIZE));

    // Callback to pass task from Download-Thread to Worker-Queue
    m_dataCallback = TileTaskCb{[&](std::shared_ptr<TileTask> task) {

        if (task->isReady()) {
             platform.requestRender();

        } else if (task->hasData()) {
            m_workers.enqueue(task);

        } else {
            task->cancel();
        }
    }};
}

TileManager::~TileManager() {
    m_tileSets.clear();
}

void TileManager::setTileSources(const std::vector<std::shared_ptr<TileSource>>& _sources) {

    LOG("setTileSources %d", _sources.size());

    m_tileCache->clear();

    // Remove all (non-client datasources) sources and respective tileSets not present in the
    // new scene
    auto it = std::remove_if(
        m_tileSets.begin(), m_tileSets.end(),
        [&](auto& tileSet) {
            if (!tileSet.clientTileSource) {
                LOGN("Remove source %s", tileSet.source->name().c_str());
                return true;
            }
            // Clear cache
            tileSet.tiles.clear();
            return false;
        });

    m_tileSets.erase(it, m_tileSets.end());

    // add new sources
    for (const auto& source : _sources) {

        // ignore sources not used to generate tile geometry
        if (!source->generateGeometry()) { continue; }

        if (std::find_if(m_tileSets.begin(), m_tileSets.end(),
                         [&](const TileSet& a) {
                             return a.source->name() == source->name();
                         }) == m_tileSets.end()) {
            LOGW("add source %s", source->name().c_str());
            m_tileSets.push_back({ source, false });
        } else {
            LOGW("Duplicate named datasource (not added): %s", source->name().c_str());
        }
    }
}

std::shared_ptr<TileSource> TileManager::getClientTileSource(int32_t sourceID) {
    for (const auto& tileSet : m_tileSets) {
        if (tileSet.clientTileSource && tileSet.source->id() == sourceID) {
            return tileSet.source;
        }
    }
    return nullptr;
}

void TileManager::addClientTileSource(std::shared_ptr<TileSource> _tileSource) {
    m_tileSets.push_back({ _tileSource, true });
}

bool TileManager::removeClientTileSource(TileSource& _tileSource) {
    bool removed = false;
    for (auto it = m_tileSets.begin(); it != m_tileSets.end();) {
        if (it->source.get() == &_tileSource) {
            // Remove the tile set associated with this tile source
            it = m_tileSets.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    return removed;
}

void TileManager::clearTileSets(bool clearSourceCaches) {

    for (auto& tileSet : m_tileSets) {
        tileSet.cancelTasks();

        tileSet.tiles.clear();

        if (clearSourceCaches) {
            tileSet.source->clearData();
        }
    }

    m_tileCache->clear();
}

void TileManager::clearTileSet(int32_t _sourceId) {
    for (auto& tileSet : m_tileSets) {
        if (tileSet.source->id() != _sourceId) { continue; }

        tileSet.cancelTasks();
        tileSet.tiles.clear();
    }

    m_tileCache->clear();
    m_tileSetChanged = true;
}

void TileManager::updateTileSets(const View& _view) {

    m_tiles.clear();
    m_tilesInProgress = 0;
    m_tileSetChanged = false;

    if (!getDebugFlag(DebugFlags::freeze_tiles)) {

        for (auto& tileSet : m_tileSets) {
            tileSet.visibleTiles.clear();
        }

        auto tileCb = [&, zoom = _view.getZoom()](TileID _tileID){
            for (auto& tileSet : m_tileSets) {
                auto zoomBias = tileSet.source->zoomBias();
                auto maxZoom = tileSet.source->maxZoom();

                // Insert scaled and maxZoom mapped tileID in the visible set
                tileSet.visibleTiles.insert(_tileID.zoomBiasAdjusted(zoomBias).withMaxSourceZoom(maxZoom));
            }
        };

        _view.getVisibleTiles(tileCb);
    }

    for (auto& tileSet : m_tileSets) {
        // check if tile set is active for zoom (zoom might be below min_zoom)
        if (tileSet.source->isActiveForZoom(_view.getZoom())) {
            updateTileSet(tileSet, _view.state());
        }
    }

    loadTiles();

    // Make m_tiles an unique list of tiles for rendering sorted from
    // high to low zoom-levels.
    std::sort(m_tiles.begin(), m_tiles.end(), [](auto& a, auto& b) {
            return a->sourceID() == b->sourceID() ?
                a->getID() < b->getID() :
                a->sourceID() < b->sourceID(); }
        );

    // Remove duplicates: Proxy tiles could have been added more than once
    m_tiles.erase(std::unique(m_tiles.begin(), m_tiles.end()), m_tiles.end());
}

void TileManager::updateTileSet(TileSet& _tileSet, const ViewState& _view) {

    bool newTiles = false;

    if (_tileSet.sourceGeneration != _tileSet.source->generation()) {
        _tileSet.sourceGeneration = _tileSet.source->generation();
    }

    // Tile load request above this zoom-level will be canceled in order to
    // not wait for tiles that are too small to contribute significantly to
    // the current view.
    int maxZoom = glm::round(_view.zoom + 1.f);
    int minZoom = glm::round(_view.zoom - 2.f);

    std::vector<TileID> removeTiles;
    auto& tiles = _tileSet.tiles;

    // Check for ready tasks, move Tile to active TileSet and unset Proxies.
    for (auto& it : tiles) {
        auto& entry = it.second;
        if (entry.completeTileTask()) {
            clearProxyTiles(_tileSet, it.first, entry, removeTiles);

            newTiles = true;
            m_tileSetChanged = true;
        }
    }

    const auto& visibleTiles = _tileSet.visibleTiles;

    // Loop over visibleTiles and add any needed tiles to tileSet
    auto curTilesIt = tiles.begin();
    auto visTilesIt = visibleTiles.begin();

    auto generation = _tileSet.source->generation();

    while (visTilesIt != visibleTiles.end() || curTilesIt != tiles.end()) {

        auto& visTileId = visTilesIt == visibleTiles.end()
            ? NOT_A_TILE : *visTilesIt;

        auto& curTileId = curTilesIt == tiles.end()
            ? NOT_A_TILE : curTilesIt->first;

        if (visTileId == curTileId) {
            // tiles in both sets match
            assert(visTilesIt != visibleTiles.end() &&
                   curTilesIt != tiles.end());

            auto& entry = curTilesIt->second;
            entry.setVisible(true);

            if (entry.tile) {
                m_tiles.push_back(entry.tile);
            } else if (entry.needsLoading()) {
                // Not yet available - enqueue for loading
                if (!entry.task) {
                    entry.task = _tileSet.source->createTask(visTileId);
                }
                enqueueTask(_tileSet, visTileId, _view);
            }

            // NB: Special handling to update tiles from ClientDataSource.
            // Can be removed once ClientDataSource is immutable
            if (entry.tile) {
                auto sourceGeneration = entry.tile->sourceGeneration();
                if ((sourceGeneration < generation) && !entry.isInProgress()) {
                    // Tile needs update - enqueue for loading
                    entry.task = _tileSet.source->createTask(visTileId);
                    enqueueTask(_tileSet, visTileId, _view);
                }
            } else if (entry.isCanceled()) {
                auto sourceGeneration = entry.task->sourceGeneration();
                if (sourceGeneration < generation) {
                    // Tile needs update - enqueue for loading
                    entry.task = _tileSet.source->createTask(visTileId);
                    enqueueTask(_tileSet, visTileId, _view);
                }
            }

            if (entry.isInProgress()) {
                m_tilesInProgress++;
            }

            if (newTiles && entry.isInProgress()) {
                // check again for proxies
                updateProxyTiles(_tileSet, visTileId, entry);
            }

            ++curTilesIt;
            ++visTilesIt;

        } else if (curTileId > visTileId) {
            // tileSet is missing an element present in visibleTiles
            // NB: if (curTileId == NOT_A_TILE) it is always > visTileId
            //     and if curTileId > visTileId, then visTileId cannot be
            //     NOT_A_TILE. (for the current implementation of > operator)
            assert(visTilesIt != visibleTiles.end());

            if (!addTile(_tileSet, visTileId)) {
                // Not in cache - enqueue for loading
                enqueueTask(_tileSet, visTileId, _view);
                m_tilesInProgress++;
            }

            ++visTilesIt;

        } else {
            // tileSet has a tile not present in visibleTiles
            assert(curTilesIt != tiles.end());

            auto& entry = curTilesIt->second;

            if (entry.getProxyCounter() > 0) {
                if (entry.tile) {
                    m_tiles.push_back(entry.tile);
                } else if (entry.isInProgress()) {
                    if (curTileId.z >= maxZoom || curTileId.z <= minZoom) {
                        // Cancel tile loading but keep tile entry for referencing
                        // this tiles proxy tiles.
                        _tileSet.source->cancelLoadingTile(*entry.task);
                        entry.clearTask();
                    }
                }
            } else {
                removeTiles.push_back(curTileId);
            }
            entry.setVisible(false);
            ++curTilesIt;
        }
    }

    while (!removeTiles.empty()) {
        auto it = tiles.find(removeTiles.back());
        removeTiles.pop_back();

        if ((it != tiles.end()) && (!it->second.isVisible()) &&
            (it->second.getProxyCounter() <= 0)) {
            clearProxyTiles(_tileSet, it->first, it->second, removeTiles);
            removeTile(_tileSet, it);
        }
    }

    for (auto& it : tiles) {
        auto& entry = it.second;

#if 0 && LOG_LEVEL >= 3
        size_t rasterLoading = 0;
        size_t rasterDone = 0;
        if (entry.task) {
            for (auto &raster : entry.task->subTasks()) {
                if (raster->isReady()) { rasterDone++; }
                else { rasterLoading++; }
            }
        }
        DBG("> %s - ready:%d proxy:%d/%d loading:%d rDone:%d rLoading:%d canceled:%d",
             it.first.toString().c_str(),
             bool(entry.tile),
             entry.getProxyCounter(),
             entry.m_proxies,
             entry.task && !entry.task->isReady(),
             rasterDone,
             rasterLoading,
             entry.task && entry.task->isCanceled());
#endif

        if (entry.isInProgress()) {
            auto& id = it.first;
            auto& task = entry.task;

            // Update tile distance to map center for load priority.
            auto tileCenter = MapProjection::tileCenter(id);
            double scaleDiv = exp2(id.z - _view.zoom);
            if (scaleDiv < 1) { scaleDiv = 0.1/scaleDiv; } // prefer parent tiles
            task->setPriority(glm::length2(tileCenter - _view.center) * scaleDiv);
            task->setProxyState(entry.getProxyCounter() > 0);
        }

        if (entry.tile) {
            // Mark as proxy
            entry.tile->setProxyState(entry.getProxyCounter() > 0);
        }
    }
}

void TileManager::enqueueTask(TileSet& _tileSet, const TileID& _tileID,
                              const ViewState& _view) {

    // Keep the items sorted by distance
    auto tileCenter = MapProjection::tileCenter(_tileID);
    double distance = glm::length2(tileCenter - _view.center);

    auto it = std::upper_bound(m_loadTasks.begin(), m_loadTasks.end(), distance,
                               [](auto& distance, auto& other){
                                   return distance < std::get<0>(other);
                               });

    m_loadTasks.insert(it, std::make_tuple(distance, &_tileSet, _tileID));
}

void TileManager::loadTiles() {

    if (m_loadTasks.empty()) { return; }

    for (auto& loadTask : m_loadTasks) {

        auto tileId = std::get<2>(loadTask);
        auto& tileSet = *std::get<1>(loadTask);
        auto tileIt = tileSet.tiles.find(tileId);
        auto& entry = tileIt->second;

        tileSet.source->loadTileData(entry.task, m_dataCallback);

        LOGTO("Load Tile: %s", tileId.toString().c_str());

    }

    // DBG("loading:%d cache: %fMB",
    //     m_loadTasks.size(),
    //     (double(m_tileCache->getMemoryUsage()) / (1024 * 1024)));

    m_loadTasks.clear();
}

bool TileManager::addTile(TileSet& _tileSet, const TileID& _tileID) {

    auto tile = m_tileCache->get(_tileSet.source->id(), _tileID);

    if (tile) {
        if (tile->sourceGeneration() == _tileSet.source->generation()) {
            m_tiles.push_back(tile);

            // Reset tile on potential internal dynamic data set
            tile->resetState();
        } else {
            // Clear stale tile data
            tile.reset();
        }
    }

    // Add TileEntry to TileSet
    auto entry = _tileSet.tiles.emplace(_tileID, tile);

    if (!tile) {
        // Add Proxy if corresponding proxy MapTile ready
        updateProxyTiles(_tileSet, _tileID, entry.first->second);

        entry.first->second.task = _tileSet.source->createTask(_tileID);
    }
    entry.first->second.setVisible(true);

    return bool(tile);
}

void TileManager::removeTile(TileSet& _tileSet, std::map<TileID, TileEntry>::iterator& _tileIt) {

    auto& entry = _tileIt->second;

    if (entry.isInProgress()) {
        // 1. Remove from Datasource. Make sure to cancel
        //  the network request associated with this tile.
        _tileSet.source->cancelLoadingTile(*entry.task);

        entry.clearTask();

    } else if (entry.tile) {
        // Add to cache
        m_tileCache->put(_tileSet.source->id(), entry.tile);
    }

    // Remove tile from set
    _tileIt = _tileSet.tiles.erase(_tileIt);
}

bool TileManager::updateProxyTile(TileSet& _tileSet, TileEntry& _tile,
                                  const TileID& _proxyTileId,
                                  const ProxyID _proxyId) {

    if (!_proxyTileId.isValid()) { return false; }

    auto& tiles = _tileSet.tiles;

    // check if the proxy exists in the visible tile set
    {
        const auto& it = tiles.find(_proxyTileId);
        if (it != tiles.end()) {
            if (_tile.setProxy(_proxyId)) {
                auto& entry = it->second;
                entry.incProxyCounter();

                if (entry.tile) {
                    m_tiles.push_back(entry.tile);
                }
                return true;
            }
            // Note: No need to check the cache: When the tile is in
            // tileSet it has already been fetched from cache
            return false;
        }
    }

    // check if the proxy exists in the cache
    {
        auto proxyTile = m_tileCache->get(_tileSet.source->id(), _proxyTileId);
        if (proxyTile && _tile.setProxy(_proxyId)) {

            auto result = tiles.emplace(_proxyTileId, proxyTile);
            auto& entry = result.first->second;
            entry.incProxyCounter();

            m_tiles.push_back(proxyTile);
            return true;
        }
    }

    return false;
}

void TileManager::updateProxyTiles(TileSet& _tileSet, const TileID& _tileID, TileEntry& _tile) {
    // TODO: this should be improved to use the nearest proxy tile available.
    // Currently it would use parent or grand*parent  as proxies even if the
    // child proxies would be more appropriate

    // Try parent proxy
    auto zoomBias = _tileSet.source->zoomBias();
    auto maxZoom = _tileSet.source->maxZoom();
    auto parentID = _tileID.getParent(zoomBias);
    auto minZoom = _tileSet.source->minDisplayZoom();
    if (minZoom <= parentID.z
            && updateProxyTile(_tileSet, _tile, parentID, ProxyID::parent)) {
        return;
    }
    // Try grandparent
    auto grandparentID = parentID.getParent(zoomBias);
    if (minZoom <= grandparentID.z
            && updateProxyTile(_tileSet, _tile, grandparentID, ProxyID::parent2)) {
        return;
    }
    // Try children
    if (maxZoom > _tileID.z) {
        for (int i = 0; i < 4; i++) {
            auto childID = _tileID.getChild(i, maxZoom);
            updateProxyTile(_tileSet, _tile, childID, static_cast<ProxyID>(1 << i));
        }
    }
}

void TileManager::clearProxyTiles(TileSet& _tileSet, const TileID& _tileID, TileEntry& _tile,
                                  std::vector<TileID>& _removes) {
    auto& tiles = _tileSet.tiles;
    auto zoomBias = _tileSet.source->zoomBias();
    auto maxZoom = _tileSet.source->maxZoom();

    auto removeProxy = [&tiles,&_removes](TileID id) {
        auto it = tiles.find(id);
        if (it != tiles.end()) {
            auto& entry = it->second;
            entry.decProxyCounter();
            if (entry.getProxyCounter() <= 0 && !entry.isVisible()) {
                _removes.push_back(id);
            }
        }
    };
    // Check if grand parent proxy is present
    if (_tile.unsetProxy(ProxyID::parent2)) {
        TileID gparentID(_tileID.getParent(zoomBias).getParent(zoomBias));
        removeProxy(gparentID);
    }

    // Check if parent proxy is present
    if (_tile.unsetProxy(ProxyID::parent)) {
        TileID parentID(_tileID.getParent(zoomBias));
        removeProxy(parentID);
    }

    // Check if child proxies are present
    for (int i = 0; i < 4; i++) {
        if (_tile.unsetProxy(static_cast<ProxyID>(1 << i))) {
            TileID childID(_tileID.getChild(i, maxZoom));
            removeProxy(childID);
        }
    }
}

void TileManager::setCacheSize(size_t _cacheSize) {
    m_tileCache->limitCacheSize(_cacheSize);
}

}
