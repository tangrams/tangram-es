#include "tile/tileManager.h"

#include "data/tileSource.h"
#include "platform.h"
#include "tile/tile.h"
#include "tile/tileCache.h"
#include "util/mapProjection.h"
#include "view/view.h"

#include "glm/gtx/norm.hpp"

#include <algorithm>

#define DBG(...) // LOGD(__VA_ARGS__)

namespace Tangram {

TileManager::TileManager(std::shared_ptr<Platform> platform, TileTaskQueue& _tileWorker) :
    m_workers(_tileWorker) {

    m_tileCache = std::unique_ptr<TileCache>(new TileCache(DEFAULT_CACHE_SIZE));

    // Callback to pass task from Download-Thread to Worker-Queue
    m_dataCallback = TileTaskCb{[this, platform](std::shared_ptr<TileTask> task) {

        if (task->isReady()) {
             platform->requestRender();

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

    m_tileCache->clear();

    // remove sources that are not in new scene - there must be a better way..
    auto it = std::remove_if(
        m_tileSets.begin(), m_tileSets.end(),
        [&](auto& tileSet) {
            if (!tileSet.clientTileSource) {
                auto sIt = std::find_if(_sources.begin(), _sources.end(),
                                        [&](auto& source){ return source->equals(*tileSet.source); });

                if (sIt == _sources.end() || !(*sIt)->generateGeometry()) {
                    LOGD("remove source %s", tileSet.source->name().c_str());
                    return true;
                }
            }
            // Clear cache
            tileSet.tiles.clear();
            return false;
        });

    m_tileSets.erase(it, m_tileSets.end());

    // add new sources
    for (const auto& source : _sources) {

        if (std::find_if(m_tileSets.begin(), m_tileSets.end(),
                         [&](const TileSet& a) {
                             return a.source->name() == source->name();
                         }) == m_tileSets.end()
                && source->generateGeometry()) {

            LOGD("add source %s", source->name().c_str());

            m_tileSets.push_back({ source, false });
        }
    }
}

void TileManager::addClientTileSource(std::shared_ptr<TileSource> _tileSource) {
    m_tileSets.push_back({ _tileSource, true });
}

bool TileManager::removeClientTileSource(TileSource& _tileSource) {
    bool removed = false;
    for (auto it = m_tileSets.begin(); it != m_tileSets.end();) {
        if (it->source.get() == &_tileSource) {
            // Remove the textures for this tile source
            it->source->clearRasters();
            // Remove the tile set associated with this tile source
            it = m_tileSets.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    return removed;
}

void TileManager::clearTileSets() {
    for (auto& tileSet : m_tileSets) {
        tileSet.tiles.clear();
    }

    m_tileCache->clear();
}

void TileManager::clearTileSet(int32_t _sourceId) {
    for (auto& tileSet : m_tileSets) {
        if (tileSet.source->id() != _sourceId) { continue; }
        tileSet.tiles.clear();
    }

    m_tileCache->clear();
    m_tileSetChanged = true;
}

void TileManager::updateTileSets(const ViewState& _view,
                                 const std::set<TileID>& _visibleTiles) {
    m_tiles.clear();
    m_tilesInProgress = 0;
    m_tileSetChanged = false;

    for (auto& tileSet : m_tileSets) {
        // check if tile set is active for zoom (zoom might be below min_zoom)
        if (tileSet.source->isActiveForZoom(_view.zoom)) {
            updateTileSet(tileSet, _view, _visibleTiles);
        }
    }

    loadTiles();

    // Make m_tiles an unique list of tiles for rendering sorted from
    // high to low zoom-levels.
    std::sort(m_tiles.begin(), m_tiles.end(), [](auto& a, auto& b){
            return a->getID() < b->getID(); });

    // Remove duplicates: Proxy tiles could have been added more than once
    m_tiles.erase(std::unique(m_tiles.begin(), m_tiles.end()), m_tiles.end());
}

void TileManager::updateTileSet(TileSet& _tileSet, const ViewState& _view,
                                const std::set<TileID>& _visibleTiles) {

    bool newTiles = false;

    if (_tileSet.sourceGeneration != _tileSet.source->generation()) {
        _tileSet.sourceGeneration = _tileSet.source->generation();
    }

    // Tile load request above this zoom-level will be canceled in order to
    // not wait for tiles that are too small to contribute significantly to
    // the current view.
    int maxZoom = _view.zoom + 2;

    std::vector<TileID> removeTiles;
    auto& tiles = _tileSet.tiles;

    // Check for ready tasks, move Tile to active TileSet and unset Proxies.
    for (auto& it : tiles) {
        auto& entry = it.second;
        if (entry.newData()) {
            clearProxyTiles(_tileSet, it.first, entry, removeTiles);
            entry.task->complete();

            entry.tile = std::move(entry.task->tile());
            entry.task.reset();
            newTiles = true;

            m_tileSetChanged = true;
        }
    }

    const auto* visibleTiles = &_visibleTiles;

    std::set<TileID> mappedTiles;
    if (_view.zoom > _tileSet.source->maxZoom()) {
        for (const auto& id : _visibleTiles) {
            auto tile = id.withMaxSourceZoom(_tileSet.source->maxZoom());
            // Replace tile with same coordinates and lower source zoom
            auto other = std::find_if(mappedTiles.begin(), mappedTiles.end(),
                             [&](auto& t) { return tile.x == t.x &&
                                            tile.y == t.y &&
                                            tile.z == t.z &&
                                            tile.wrap == t.wrap; });
            if (other == mappedTiles.end()) {
                mappedTiles.insert(tile);
            } else if (other->s < tile.s) {
                mappedTiles.erase(other);
                mappedTiles.insert(tile);
            }
        }
        visibleTiles = &mappedTiles;
    }

    // Loop over visibleTiles and add any needed tiles to tileSet
    auto curTilesIt = tiles.begin();
    auto visTilesIt = visibleTiles->begin();

    auto generation = _tileSet.source->generation();

    while (visTilesIt != visibleTiles->end() || curTilesIt != tiles.end()) {

        auto& visTileId = visTilesIt == visibleTiles->end()
            ? NOT_A_TILE : *visTilesIt;

        auto& curTileId = curTilesIt == tiles.end()
            ? NOT_A_TILE : curTilesIt->first;

        if (visTileId == curTileId) {
            // tiles in both sets match
            assert(visTilesIt != visibleTiles->end() &&
                   curTilesIt != tiles.end());

            auto& entry = curTilesIt->second;
            entry.setVisible(true);

            auto sourceGeneration = (entry.isReady()) ?
                entry.tile->sourceGeneration() : entry.task->sourceGeneration();

            if (entry.isReady()) {
                m_tiles.push_back(entry.tile);

                if (!entry.isInProgress() &&
                    (sourceGeneration < generation)) {
                    // Tile needs update - enqueue for loading
                    entry.task = _tileSet.source->createTask(visTileId);
                    enqueueTask(_tileSet, visTileId, _view);
                }
            } else if (entry.needsLoading()) {
                // Not yet available - enqueue for loading
                enqueueTask(_tileSet, visTileId, _view);

            } else if (entry.isCanceled() &&
                       (sourceGeneration < generation)) {
                // Tile needs update - enqueue for loading
                entry.task = _tileSet.source->createTask(visTileId);
                enqueueTask(_tileSet, visTileId, _view);
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
            assert(visTilesIt != visibleTiles->end());

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
                if (entry.isReady()) {
                    m_tiles.push_back(entry.tile);
                } else if (curTileId.z < maxZoom) {
                    // Cancel loading
                    removeTiles.push_back(curTileId);
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

        if ((it != tiles.end()) &&
            (!it->second.isVisible()) &&
            (it->second.getProxyCounter() <= 0  ||
             it->first.z >= maxZoom)) {

            clearProxyTiles(_tileSet, it->first, it->second, removeTiles);

            removeTile(_tileSet, it);
        }
    }

    for (auto& it : tiles) {
        auto& entry = it.second;

#if LOG_LEVEL >= 3
        size_t rasterLoading = 0;
        size_t rasterDone = 0;
        if (entry.task) {
            for (auto &raster : entry.task->subTasks()) {
                if (raster->isReady()) { rasterDone++; }
                else { rasterLoading++; }
            }
        }
        DBG("> %s - ready:%d proxy:%d/%d loading:%d rDone:%d rLoading:%d rPending:%d canceled:%d",
             it.first.toString().c_str(),
             entry.isReady(),
             entry.getProxyCounter(),
             entry.m_proxies,
             entry.task && !entry.task->isReady(),
             rasterDone,
             rasterLoading,
             entry.rastersPending(),
             entry.task && entry.task->isCanceled());
#endif

        if (entry.isInProgress()) {
            auto& id = it.first;
            auto& task = entry.task;

            // Update tile distance to map center for load priority.
            auto tileCenter = _view.mapProjection->TileCenter(id);
            double scaleDiv = exp2(id.z - _view.zoom);
            if (scaleDiv < 1) { scaleDiv = 0.1/scaleDiv; } // prefer parent tiles
            task->setPriority(glm::length2(tileCenter - _view.center) * scaleDiv);
            task->setProxyState(entry.getProxyCounter() > 0);
        }

        if (entry.isReady()) {
            // Mark as proxy
            entry.tile->setProxyState(entry.getProxyCounter() > 0);
        }
    }
}

void TileManager::enqueueTask(TileSet& _tileSet, const TileID& _tileID,
                              const ViewState& _view) {

    // Keep the items sorted by distance
    auto tileCenter = _view.mapProjection->TileCenter(_tileID);
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
    }

    DBG("loading:%d pending:%d cache: %fMB",
        m_loadTasks.size(), m_loadPending,
        (double(m_tileCache->getMemoryUsage()) / (1024 * 1024)));

    m_loadTasks.clear();
}

bool TileManager::addTile(TileSet& _tileSet, const TileID& _tileID) {

    auto tile = m_tileCache->get(_tileSet.source->id(), _tileID);

    if (tile) {
        if (tile->sourceGeneration() == _tileSet.source->generation()) {
            m_tiles.push_back(tile);

            // Update tile origin based on wrap (set in the new tileID)
            tile->updateTileOrigin(_tileID.wrap);

            // Reset tile on potential internal dynamic data set
            // TODO rename to resetState() to avoid ambiguity
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

    auto& id = _tileIt->first;
    auto& entry = _tileIt->second;


    if (entry.isInProgress()) {
        entry.clearTask();

        // 1. Remove from Datasource. Make sure to cancel
        //  the network request associated with this tile.
        _tileSet.source->cancelLoadingTile(id);

    } else if (entry.isReady()) {
        // Add to cache
        auto poppedTiles = m_tileCache->put(_tileSet.source->id(), entry.tile);
        for (auto& tileID : poppedTiles) {
            _tileSet.source->clearRaster(tileID);
        }
    }

    // Remove rasters from this TileSource
    _tileSet.source->clearRaster(id);

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
            auto& entry = it->second;

            if (!entry.isCanceled() && _tile.setProxy(_proxyId)) {
                entry.incProxyCounter();

                if (entry.isReady()) {
                    m_tiles.push_back(entry.tile);
                }

                // Note: No need to check the cache: When the tile is in
                // tileSet it would have already been fetched from cache
                return true;
            }
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
    auto parentID = _tileID.getParent();
    auto minZoom = _tileSet.source->minDisplayZoom();
    if (minZoom <= parentID.z
            && updateProxyTile(_tileSet, _tile, parentID, ProxyID::parent)) {
        return;
    }
    // Try grandparent
    auto grandparentID = parentID.getParent();
    if (minZoom <= grandparentID.z
            && updateProxyTile(_tileSet, _tile, grandparentID, ProxyID::parent2)) {
        return;
    }
    // Try children
    if (_tileSet.source->maxZoom() > _tileID.z) {
        for (int i = 0; i < 4; i++) {
            auto childID = _tileID.getChild(i, _tileSet.source->maxZoom());
            updateProxyTile(_tileSet, _tile, childID, static_cast<ProxyID>(1 << i));
        }
    }
}

void TileManager::clearProxyTiles(TileSet& _tileSet, const TileID& _tileID, TileEntry& _tile,
                                  std::vector<TileID>& _removes) {
    auto& tiles = _tileSet.tiles;

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
        TileID gparentID(_tileID.getParent().getParent());
        removeProxy(gparentID);
    }

    // Check if parent proxy is present
    if (_tile.unsetProxy(ProxyID::parent)) {
        TileID parentID(_tileID.getParent());
        removeProxy(parentID);
    }

    // Check if child proxies are present
    for (int i = 0; i < 4; i++) {
        if (_tile.unsetProxy(static_cast<ProxyID>(1 << i))) {
            TileID childID(_tileID.getChild(i));
            removeProxy(childID);
        }
    }
}

void TileManager::setCacheSize(size_t _cacheSize) {
    m_tileCache->limitCacheSize(_cacheSize);
}

}
