#include "tileManager.h"

#include "data/dataSource.h"
#include "platform.h"
#include "tile/tile.h"
#include "view/view.h"
#include "tileCache.h"

#include "glm/gtx/norm.hpp"

#include <chrono>
#include <algorithm>

namespace Tangram {

TileManager::TileManager(TileTaskQueue& _tileWorker) : m_workers(_tileWorker) {

    m_tileCache = std::unique_ptr<TileCache>(new TileCache(DEFAULT_CACHE_SIZE));

    m_dataCallback = TileTaskCb{[this](std::shared_ptr<TileTask>&& task) {
            if (task->hasData()) {
                m_workers.enqueue(std::move(task));
            } else {
                LOGD("No data for tile: %s", task->tile()->getID().toString().c_str());
                task->cancel();
            }
    }};
}

TileManager::~TileManager() {
    m_tileSets.clear();
}

void TileManager::setDataSources(std::vector<std::shared_ptr<DataSource>> _sources) {
    m_tileCache->clear();

    // remove sources that are not in new scene - there must be a better way..
    auto it = std::remove_if(
        m_tileSets.begin(), m_tileSets.end(),
        [&](auto& tileSet) {
            auto sIt = std::find_if(
                _sources.begin(), _sources.end(),
                [&](auto& s){ return tileSet.source->equals(*s); });

            if (sIt == _sources.end()) {
                LOG("remove source %s", tileSet.source->name().c_str());
                return true;
            }

            // Cancel pending  tiles
            for_each(tileSet.tiles.begin(), tileSet.tiles.end(), [&](auto& tile) {
                    tile.second.cancelTask();
                });

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
                         }) == m_tileSets.end()) {
            LOG("add source %s", source->name().c_str());
            addDataSource(source);
        }
    }
}

void TileManager::addDataSource(std::shared_ptr<DataSource> _dataSource) {
    m_tileSets.push_back({ _dataSource });
}

void TileManager::clearTileSets() {
    for (auto& tileSet : m_tileSets) {
        for (auto& tile : tileSet.tiles) {
            tile.second.cancelTask();
        }
        tileSet.tiles.clear();
    }

    m_tileCache->clear();
    m_loadPending = 0;
}

void TileManager::clearTileSet(int32_t _sourceId) {
    for (auto& tileSet : m_tileSets) {
        if (tileSet.source->id() != _sourceId) { continue; }
        for (auto& tile : tileSet.tiles) {
            tile.second.cancelTask();
        }
        tileSet.tiles.clear();
    }

    m_tileCache->clear();
    m_loadPending = 0;
    m_tileSetChanged = true;
}

void TileManager::updateTileSets() {

    m_tileSetChanged = false;
    m_tiles.clear();
    m_loadPending = 0;

    for (auto& tileSet : m_tileSets) {
        updateTileSet(tileSet);
    }

    loadTiles();

    // Make m_tiles an unique list of tiles for rendering sorted from
    // high to low zoom-levels.
    std::sort(m_tiles.begin(), m_tiles.end(), [](auto& a, auto& b){
            return a->getID() < b->getID(); });

    // Remove duplicates: Proxy tiles could have been added more than once
    m_tiles.erase(std::unique(m_tiles.begin(), m_tiles.end()), m_tiles.end());
}

void TileManager::updateTileSet(TileSet& _tileSet) {

    auto& tiles = _tileSet.tiles;

    std::vector<TileID> removeTiles;

    const std::set<TileID>& visibleTiles = m_view->getVisibleTiles();

    glm::dvec2 viewCenter(m_view->getPosition().x, -m_view->getPosition().y);

    // Tile load request above this zoom-level will be canceled in order to
    // not wait for tiles that are too small to contribute significantly to
    // the current view.
    // int maxZoom = m_view->getZoom() + 2;

    m_tileSetChanged |= m_workers.checkProcessedTiles();

    if (_tileSet.sourceGeneration != _tileSet.source->generation()) {
        _tileSet.sourceGeneration = _tileSet.source->generation();
        m_tileSetChanged = true;
    }

    if (m_view->changedOnLastUpdate() || m_tileSetChanged) {

        // Loop over visibleTiles and add any needed tiles to tileSet
        auto curTilesIt = tiles.begin();
        auto visTilesIt = visibleTiles.begin();

        while (visTilesIt != visibleTiles.end() || curTilesIt != tiles.end()) {

            auto& visTileId = visTilesIt == visibleTiles.end()
                ? NOT_A_TILE
                : *visTilesIt;

            auto& curTileId = curTilesIt == tiles.end()
                ? NOT_A_TILE
                : curTilesIt->first;

            if (visTileId == curTileId) {
                // tiles in both sets match
                assert(!(visTileId == NOT_A_TILE));

                auto& entry = curTilesIt->second;
                entry.setVisible(true);

                if (entry.newData()) {
                    clearProxyTiles(_tileSet, curTileId, entry, removeTiles);
                    entry.tile = std::move(entry.task->tile());
                    entry.task.reset();
                }

                if (entry.isReady()) {
                    m_tiles.push_back(entry.tile);

                    bool update = !entry.isLoading() &&
                        (entry.tile->sourceGeneration() < _tileSet.source->generation());

                    if (update) {
                        LOG("reload tile %s - %d %d", visTileId.toString().c_str(),
                            entry.tile->sourceGeneration(),
                            _tileSet.source->generation());

                        // Tile needs update - enqueue for loading
                        enqueueTask(_tileSet, visTileId, viewCenter);
                    }

                } else if (!entry.isLoading()) {

                    // Not yet available - enqueue for loading
                    enqueueTask(_tileSet, visTileId, viewCenter);

                    if (m_tileSetChanged) {
                        // check again for proxies
                        updateProxyTiles(_tileSet, visTileId, entry);
                    }
                }

                ++curTilesIt;
                ++visTilesIt;

            } else if (curTileId > visTileId) {
                // tileSet is missing an element present in visibleTiles
                // NB: if (curTileId == NOT_A_TILE) it is always > visTileId
                //     and if curTileId > visTileId, then visTileId cannot be
                //     NOT_A_TILE. (for the current implementation of > operator)
                assert(!(visTileId == NOT_A_TILE));

                if (!addTile(_tileSet, visTileId)) {
                    // Not in cache - enqueue for loading
                    enqueueTask(_tileSet, visTileId, viewCenter);
                }

                ++visTilesIt;

            } else {
                // tileSet has a tile not present in visibleTiles
                assert(!(curTileId == NOT_A_TILE));
                auto& entry = curTilesIt->second;

                if (entry.getProxyCounter() > 0) {
                    if (entry.newData()) {
                        clearProxyTiles(_tileSet, curTileId, entry, removeTiles);
                        entry.tile = std::move(entry.task->tile());
                        entry.task.reset();
                    }
                    if (entry.isReady()) {
                        m_tiles.push_back(entry.tile);
                    }
                } else {
                    removeTiles.push_back(curTileId);
                }
                entry.setVisible(false);
                ++curTilesIt;
            }
        }
    }

    while (!removeTiles.empty()) {
        auto it = tiles.find(removeTiles.back());
        removeTiles.pop_back();

        if ((it != tiles.end()) &&
            (it->second.getProxyCounter() <= 0)) { // ||
            //tileIt->first.z > maxZoom)) {

            auto& entry = it->second;

            LOG("REMOVE %s - ready:%d proxy:%d visible:%d - %d",
                it->first.toString().c_str(),
                entry.isReady(),
                entry.getProxyCounter(),
                entry.isVisible(),
                entry.m_proxies);

            clearProxyTiles(_tileSet, it->first, it->second, removeTiles);

            removeTile(_tileSet, it);
        }
    }

    for (auto it = tiles.begin(); it != tiles.end();) {
        auto& entry = it->second;

        LOG("> %s - ready:%d proxy:%d/%d visible:%d loading:%d",
            it->first.toString().c_str(),
            entry.isReady(),
            entry.getProxyCounter(),
            entry.m_proxies,
            entry.isVisible(),
            entry.task && !entry.task->hasData()
            );

        if (entry.isLoading()) {
            auto& id = it->first;
            auto& task = entry.task;

            // Update tile distance to map center for load priority.
            auto tileCenter = m_view->getMapProjection().TileCenter(id);
            double scaleDiv = exp2(id.z - m_view->getZoom());
            if (scaleDiv < 1) { scaleDiv = 0.1/scaleDiv; } // prefer parent tiles
            task->setPriority(glm::length2(tileCenter - viewCenter) * scaleDiv);
            //task->visible = entry.isVisible();

            // Count tiles that are currently being downloaded to
            // limit download requests.
            if (!task->hasData()) {

                m_loadPending++;
            }
        }

        if (entry.isReady()) {
            // Mark as proxy
            entry.tile->setProxyState(entry.getProxyCounter() > 0);
        }
        ++it;
    }
}

void TileManager::enqueueTask(TileSet& _tileSet, const TileID& _tileID,
                              const glm::dvec2& _viewCenter) {

    // Keep the items sorted by distance
    auto tileCenter = m_view->getMapProjection().TileCenter(_tileID);
    double distance = glm::length2(tileCenter - _viewCenter);

    auto it = std::upper_bound(m_loadTasks.begin(), m_loadTasks.end(), distance,
                               [](auto& distance, auto& other){
                                   return distance < std::get<0>(other);
                               });

    m_loadTasks.insert(it, std::make_tuple(distance, &_tileSet, &_tileID));
}

void TileManager::loadTiles() {

    for (auto& loadTask : m_loadTasks) {

        auto tileId = *std::get<2>(loadTask);
        auto& tileSet = *std::get<1>(loadTask);
        auto tileIt = tileSet.tiles.find(tileId);
        auto& entry = tileIt->second;

        auto task = tileSet.source->createTask(tileId);

        if (task->hasData()) {
            // NB: Set implicit 'loading' state
            entry.task = task;
            m_dataCallback.func(std::move(task));

        } else if (m_loadPending < int(MAX_DOWNLOADS)) {
            entry.task = task;
            if (tileSet.source->loadTileData(std::move(task), m_dataCallback)) {
                m_loadPending++;
            } else {
                // TODO: How to handle this case?
                // This would constantly try to reload the failed tile:
                // entry.task.reset()
            }
        }
    }

    LOG("loading:%d pending:%d cache: %fMB",
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
    }
    entry.first->second.setVisible(true);

    return bool(tile);
}

void TileManager::removeTile(TileSet& _tileSet, std::map<TileID, TileEntry>::iterator& _tileIt) {

    auto& id = _tileIt->first;
    auto& entry = _tileIt->second;

    // clearProxyTiles(tileSet, id, entry);

    if (entry.isLoading()) {
        entry.cancelTask();

        // 1. Remove from Datasource. Make sure to cancel
        //  the network request associated with this tile.
        _tileSet.source->cancelLoadingTile(id);

    } else if (entry.isReady()) {
        // Add to cache
        m_tileCache->put(_tileSet.source->id(), entry.tile);
    }

    // Remove tile from set
    _tileIt = _tileSet.tiles.erase(_tileIt);
}

bool TileManager::updateProxyTile(TileSet& _tileSet, TileEntry& _tile,
                                  const TileID& _proxyTileId,
                                  const ProxyID _proxyId) {
    auto& tiles = _tileSet.tiles;

    // check if the proxy exists in the visible tile set
    {
        const auto& it = tiles.find(_proxyTileId);
        if (it != tiles.end()) {
            // FIXME: this uses indirect proxy tiles
            // even if a cached tile for the direct proxy
            // may be available
            auto& entry = it->second;
            if (_tile.setProxy(_proxyId)) {
                entry.incProxyCounter();

                if (entry.isReady()) {
                    m_tiles.push_back(entry.tile);
                }
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

    // Try parent proxy
    auto parentID = _tileID.getParent();
    if (updateProxyTile(_tileSet, _tile, parentID, ProxyID::parent)) {
        LOG("use parent proxy");
        return;
    }
    // Try grandparent
    if (updateProxyTile(_tileSet, _tile, parentID.getParent(), ProxyID::parent2)) {
        LOG("use grand parent proxy");
        return;
    }
    // Try children
    if (m_view->s_maxZoom > _tileID.z) {
        for (int i = 0; i < 4; i++) {
            if (updateProxyTile(_tileSet, _tile, _tileID.getChild(i), static_cast<ProxyID>(1 << i))) {
                LOG("use child proxy");
            }
        }
    }
}

void TileManager::clearProxyTiles(TileSet& _tileSet, const TileID& _tileID, TileEntry& _tile,
                                  std::vector<TileID>& _removes) {
    auto& tiles = _tileSet.tiles;

    // Check if grand parent proxy is present
    if (_tile.unsetProxy(ProxyID::parent2)) {
        TileID gparentID(_tileID.getParent().getParent());
        auto it = tiles.find(gparentID);
        if (it != tiles.end()) {
            it->second.decProxyCounter();
            if (it->second.getProxyCounter() <= 0) {
                _removes.push_back(gparentID);
            }
        }
    }

    // Check if parent proxy is present
    if (_tile.unsetProxy(ProxyID::parent)) {
        TileID parentID(_tileID.getParent());
        auto it = tiles.find(parentID);
        if (it != tiles.end()) {
            it->second.decProxyCounter();
            if (it->second.getProxyCounter() <= 0) {
                _removes.push_back(parentID);
            }
        }
    }

    // Check if child proxies are present
    for (int i = 0; i < 4; i++) {
        if (_tile.unsetProxy(static_cast<ProxyID>(1 << i))) {
            TileID childID(_tileID.getChild(i));
            auto it = tiles.find(childID);
            if (it != tiles.end()) {
                it->second.decProxyCounter();
                if (it->second.getProxyCounter() <= 0) {
                    _removes.push_back(childID);
                }
            }
        }
    }
}

void TileManager::setCacheSize(size_t _cacheSize) {
    m_tileCache->limitCacheSize(_cacheSize);
}

}
