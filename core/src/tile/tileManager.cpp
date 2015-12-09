#include "tileManager.h"

#include "data/dataSource.h"
#include "platform.h"
#include "scene/scene.h"
#include "tile/tile.h"
#include "view/view.h"
#include "tileCache.h"

#include "glm/gtx/norm.hpp"

#include <chrono>
#include <algorithm>

namespace Tangram {

TileManager::TileManager() {

    // Instantiate workers
    m_workers = std::unique_ptr<TileWorker>(new TileWorker(*this, MAX_WORKERS));

    m_tileCache = std::unique_ptr<TileCache>(new TileCache(DEFAULT_CACHE_SIZE));

    m_dataCallback = TileTaskCb{[this](std::shared_ptr<TileTask>&& task) {
            if (task->loaded) {
                m_workers->enqueue(std::move(task));
            } else {
                LOGD("No data for tile: %s", task->tile->getID().toString().c_str());
                task->cancel();
            }
    }};
}

TileManager::~TileManager() {
    if (m_workers->isRunning()) {
        m_workers->stop();
    }
    m_tileSets.clear();
}

void TileManager::setScene(std::shared_ptr<Scene> _scene) {
    m_tileCache->clear();

    auto& sources = _scene->dataSources();

    // remove sources that are not in new scene - there must be a better way..
    auto it = std::remove_if(
        m_tileSets.begin(), m_tileSets.end(),
        [&](auto& tileSet) {
            auto sIt = std::find_if(
                sources.begin(), sources.end(),
                [&](auto& s){ return tileSet.source->equals(*s); });

            if (sIt == sources.end()) {
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
    for (const auto& source : sources) {

        if (std::find_if(m_tileSets.begin(), m_tileSets.end(),
                         [&](const TileSet& a) {
                             return a.source->name() == source->name();
                         }) == m_tileSets.end()) {
            LOG("add source %s", source->name().c_str());
            addDataSource(source);
        }
    }

    m_scene = _scene;
}

void TileManager::addDataSource(std::shared_ptr<DataSource> dataSource) {
    m_tileSets.push_back({ dataSource });
}

void TileManager::tileProcessed(std::shared_ptr<TileTask>&& _task) {
    std::lock_guard<std::mutex> lock(m_readyTileMutex);
    m_readyTiles.push_back(std::move(_task));
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

    m_tiles.erase(std::unique(m_tiles.begin(), m_tiles.end()), m_tiles.end());
}

void TileManager::updateTileSet(TileSet& tileSet) {

    auto& tiles = tileSet.tiles;

    std::vector<TileID> removeTiles;

    if (!m_readyTiles.empty()) {
        std::lock_guard<std::mutex> lock(m_readyTileMutex);
        auto taskIt = m_readyTiles.begin();

        while (taskIt != m_readyTiles.end()) {

            if (tileSet.source != (*taskIt)->source) {
                taskIt++;
                continue;
            }
            auto task = *taskIt;
            taskIt = m_readyTiles.erase(taskIt);

            auto setTile = tileSet.tiles.find(task->tileId);
            if (setTile == tileSet.tiles.end()) { continue; }

            auto& entry = setTile->second;

            if (!task->tile) {
                LOG("got empty tile task %d", task->isCanceled());
                entry.task.reset();
                continue;
            }

            if (!entry.isLoading()) {
                // FIXME: just testing: this case should not happen
                LOG("got orphaned tile task");
                assert(entry.m_proxies == 0);
                continue;
            }

            // NB: Unset implicit 'loading' state
            entry.task.reset();

            if (entry.isReady() &&
                entry.tile->sourceGeneration() > task->tile->sourceGeneration()) {
                // FIXME: just testing: this case should be avoided
                LOG("got older tile task");
                assert(entry.m_proxies == 0);
                continue;
            }

            if (entry.isReady()) {
                LOG("%s replace tile %d -> %d",
                    task->tile->getID().toString().c_str(),
                    entry.tile->sourceGeneration(),
                    task->tile->sourceGeneration());
            } else {
                LOG("%s new tile %d",
                    task->tile->getID().toString().c_str(),
                    task->tile->sourceGeneration());
            }

            clearProxyTiles(tileSet, setTile->first, entry, removeTiles);
            entry.tile = task->tile;
            m_tileSetChanged = true;
        }
    }

    const std::set<TileID>& visibleTiles = m_view->getVisibleTiles();

    glm::dvec2 viewCenter(m_view->getPosition().x, -m_view->getPosition().y);

    // Tile load request above this zoom-level will be canceled in order to
    // not wait for tiles that are too small to contribute significantly to
    // the current view.
    //int maxZoom = m_view->getZoom() + 2;

    if (tileSet.sourceGeneration != tileSet.source->generation()) {
        tileSet.sourceGeneration = tileSet.source->generation();
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

                if (entry.isReady()) {
                    m_tiles.push_back(entry.tile);

                    bool update = !entry.isLoading() &&
                        (entry.tile->sourceGeneration() < tileSet.source->generation());

                    if (update) {
                        LOG("reload tile %s - %d %d", visTileId.toString().c_str(),
                            entry.tile->sourceGeneration(),
                            tileSet.source->generation());

                        // Tile needs update - enqueue for loading
                        enqueueTask(tileSet, visTileId, viewCenter);
                    }

                } else if (!entry.isLoading()) {

                    // Not yet available - enqueue for loading
                    enqueueTask(tileSet, visTileId, viewCenter);

                    if (m_tileSetChanged) {
                        // check again for proxies
                        updateProxyTiles(tileSet, visTileId, entry);
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

                if (!addTile(tileSet, visTileId)) {
                    // Not in cache - enqueue for loading
                    enqueueTask(tileSet, visTileId, viewCenter);
                }

                ++visTilesIt;

            } else {
                // tileSet has a tile not present in visibleTiles
                assert(!(curTileId == NOT_A_TILE));
                auto& entry = curTilesIt->second;

                if (entry.getProxyCounter() > 0) {
                    if (entry.isReady()) {
                        m_tiles.push_back(entry.tile);
                    }
                } else {
                    LOG("CLEANUP PROXY %s", curTileId.toString().c_str());
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
            (!it->second.isVisible()) &&
            (it->second.getProxyCounter() <= 0)) { // ||
            //tileIt->first.z > maxZoom)) {

            auto& entry = it->second;

            LOG("REMOVE %s - ready:%d proxy:%d/%d",
                it->first.toString().c_str(),
                entry.isReady(),
                entry.getProxyCounter(),
                entry.m_proxies);

            clearProxyTiles(tileSet, it->first, it->second, removeTiles);

            removeTile(tileSet, it);
        }
    }

    for (auto it = tiles.begin(); it != tiles.end();) {
        auto& entry = it->second;

        LOG("> %s - ready:%d proxy:%d/%d loading:%d",
            it->first.toString().c_str(),
            entry.isReady(),
            entry.getProxyCounter(),
            entry.m_proxies,
            entry.task && !entry.task->loaded
            );

        if (entry.isLoading()) {
            auto& id = it->first;
            auto& task = entry.task;

            // Update tile distance to map center for load priority.
            auto tileCenter = m_view->getMapProjection().TileCenter(id);
            double scaleDiv = exp2(id.z - m_view->getZoom());
            if (scaleDiv < 1) { scaleDiv = 0.1/scaleDiv; } // prefer parent tiles
            task->setPriority(glm::length2(tileCenter - viewCenter) * scaleDiv);

            // Count tiles that are currently being downloaded to
            // limit download requests.
            if (!task->loaded) {

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

void TileManager::enqueueTask(TileSet& tileSet, const TileID& tileID,
                              const glm::dvec2& viewCenter) {

    // Keep the items sorted by distance
    auto tileCenter = m_view->getMapProjection().TileCenter(tileID);
    double distance = glm::length2(tileCenter - viewCenter);

    auto it = std::upper_bound(m_loadTasks.begin(), m_loadTasks.end(), distance,
                               [](auto& distance, auto& other){
                                   return distance < std::get<0>(other);
                               });

    m_loadTasks.insert(it, std::make_tuple(distance, &tileSet, &tileID));
}

void TileManager::loadTiles() {

    for (auto& loadTask : m_loadTasks) {

        auto tileId = *std::get<2>(loadTask);
        auto& tileSet = *std::get<1>(loadTask);
        auto tileIt = tileSet.tiles.find(tileId);
        auto& entry = tileIt->second;
        auto task = std::make_shared<TileTask>(tileId, tileSet.source);

        bool cached = tileSet.source->getTileData(task);

        if (cached || m_loadPending < int(MAX_DOWNLOADS)) {
            // NB: Set implicit 'loading' state
            entry.task = task;

            if (cached) {
                m_dataCallback.func(std::move(task));

            } else if (tileSet.source->loadTileData(std::move(task), m_dataCallback)) {
                m_loadPending++;
            }
        }
    }

    LOG("loading:%d pending:%d cache: %fMB",
       m_loadTasks.size(), m_loadPending,
       (double(m_tileCache->getMemoryUsage()) / (1024 * 1024)));

    m_loadTasks.clear();
}

bool TileManager::addTile(TileSet& tileSet, const TileID& _tileID) {

    auto tile = m_tileCache->get(tileSet.source->id(), _tileID);

    if (tile) {
        if (tile->sourceGeneration() == tileSet.source->generation()) {
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
    auto entry = tileSet.tiles.emplace(_tileID, tile);

    if (!tile) {
        // Add Proxy if corresponding proxy MapTile ready
        updateProxyTiles(tileSet, _tileID, entry.first->second);
    }
    entry.first->second.setVisible(true);

    return bool(tile);
}

void TileManager::removeTile(TileSet& tileSet, std::map<TileID, TileEntry>::iterator& _tileIt) {

    auto& id = _tileIt->first;
    auto& entry = _tileIt->second;


    if (entry.isLoading()) {
        entry.cancelTask();

        // 1. Remove from Datasource. Make sure to cancel
        //  the network request associated with this tile.
        tileSet.source->cancelLoadingTile(id);

    } else if (entry.isReady()) {
        // Add to cache
        m_tileCache->put(tileSet.source->id(), entry.tile);
    }

    // Remove tile from set
    _tileIt = tileSet.tiles.erase(_tileIt);
}

bool TileManager::updateProxyTile(TileSet& tileSet, TileEntry& _tile, const TileID& _proxyTileId,
                                  const ProxyID _proxyId) {
    auto& tiles = tileSet.tiles;

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

                // Note: No need to check the cache: When the tile is in
                // tileSet it would have already been fetched from cache
                return true;
            }
        }
    }

    // check if the proxy exists in the cache
    {
        auto proxyTile = m_tileCache->get(tileSet.source->id(), _proxyTileId);
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

void TileManager::updateProxyTiles(TileSet& tileSet, const TileID& _tileID, TileEntry& _tile) {

    // Try parent proxy
    auto parentID = _tileID.getParent();
    if (updateProxyTile(tileSet, _tile, parentID, ProxyID::parent)) {
        LOG("use parent proxy");
        return;
    }
    // Try grandparent
    if (updateProxyTile(tileSet, _tile, parentID.getParent(), ProxyID::parent2)) {
        LOG("use grand parent proxy");
        return;
    }
    // Try children
    if (m_view->s_maxZoom > _tileID.z) {
        for (int i = 0; i < 4; i++) {
            if (updateProxyTile(tileSet, _tile, _tileID.getChild(i), static_cast<ProxyID>(1 << i))) {
                LOG("use child proxy");
            }
        }
    }
}

void TileManager::clearProxyTiles(TileSet& tileSet, const TileID& _tileID, TileEntry& _tile,
                                  std::vector<TileID>& _removes) {
    auto& tiles = tileSet.tiles;

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
