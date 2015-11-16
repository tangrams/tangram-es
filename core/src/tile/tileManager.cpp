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

TileManager::TileManager()
    : m_loadPending(0) {

    // Instantiate workers
    m_workers = std::unique_ptr<TileWorker>(new TileWorker(*this, MAX_WORKERS));

    m_tileCache = std::unique_ptr<TileCache>(new TileCache(DEFAULT_CACHE_SIZE));

    m_dataCallback = TileTaskCb{[this](std::shared_ptr<TileTask>&& task){
        if (!task->loaded) {
            LOGD("No data for tile: %s", task->tile->getID().toString().c_str());

            // Set 'canceled' state when no data was received, when state is
            // already 'canceled' the state remains canceled.
            setTileState(*task->tile, TileState::canceled);
        }
        else if (setTileState(*task->tile, TileState::processing)) {
            // Enqueue tile for processing by TileWorker
            m_workers->enqueue(std::move(task));
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
                    this->setTileState(*tile.second, TileState::canceled); });

            // Clear cache
            tileSet.tiles.clear();
            return false;
        });

    m_tileSets.erase(it, m_tileSets.end());

    // add new sources
    for (const auto& source : sources) {

        if(std::find_if(
               m_tileSets.begin(), m_tileSets.end(),
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

bool TileManager::setTileState(Tile& _tile, TileState _newState) {
    std::lock_guard<std::mutex> lock(m_tileStateMutex);

    switch (_tile.getState()) {
    case TileState::none:
        if (_newState == TileState::loading) {
            _tile.setState(_newState);
            m_loadPending++;
            return true;
        }
        if (_newState == TileState::processing ||
            _newState == TileState::stale) {
            _tile.setState(_newState);
            return true;
        }
        if (_newState == TileState::none) {
            return true;
        }
        break;

    case TileState::loading:
        if (_newState == TileState::processing ||
            _newState == TileState::canceled ||
            _newState == TileState::stale) {
            _tile.setState(_newState);
            m_loadPending--;
            return true;
        }
        break;

    case TileState::processing:
        if (_newState == TileState::ready ||
                _newState == TileState::stale) {
            _tile.setState(_newState);
            return true;
        }
        break;

    case TileState::canceled:
        // Ignore any state change when tile was canceled
        return false;

    case TileState::stale:
        if (_newState == TileState::updating) {
            _tile.setState(_newState);
            return true;
        }

    case TileState::updating:
        if (_newState == TileState::ready ||
            _newState == TileState::stale ||
            _newState == TileState::canceled) {
            _tile.setState(_newState);
            return true;
        }

    case TileState::ready:
        if (_newState == TileState::stale) {
            _tile.setState(_newState);
            return true;
        }

    default:
        break;
    }

    if (_newState == TileState::canceled) {
        _tile.setState(_newState);
        return true;
    }

    LOGE("Wrong state change %d -> %d<<<", _tile.getState(), _newState);
    assert(false);
    return false; // ...
}

void TileManager::clearTileSets() {
    for (auto& tileSet : m_tileSets) {
        for (auto& tile : tileSet.tiles) {
            setTileState(*tile.second, TileState::canceled);
        }
        tileSet.tiles.clear();
    }

    m_tileCache->clear();
    m_loadPending = 0;
}

void TileManager::clearTileSet(int32_t _id) {
    for (auto& tileSet : m_tileSets) {
        if (tileSet.source->id() != _id) { continue; }
        for (auto& tile : tileSet.tiles) {
            setTileState(*tile.second, TileState::canceled);
        }
        tileSet.tiles.clear();
    }

    m_tileCache->clear();
    m_loadPending = 0;
    m_tileSetChanged = true;
}

void TileManager::markStale(int32_t _id) {
    for (auto& tileSet : m_tileSets) {
        if (tileSet.source->id() != _id) { continue; }
        for (auto& tile : tileSet.tiles) {
            setTileState(*tile.second, TileState::stale);
        }
    }
}

void TileManager::updateTileSets() {

    m_tileSetChanged = false;
    m_tiles.clear();

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
        auto it = m_readyTiles.begin();

        while (it != m_readyTiles.end()) {
            auto& task = *it;
            auto& tile = task->tile; //task tile, could be a non tileSet tile to refresh a stale tile

            if (tileSet.source == task->source) {

                auto& setTile = tileSet.tiles[tile->getID()];
                if (setTile && setTile->hasState(TileState::updating)) {
                    setTile = tile;
                    m_tileSetChanged = true;
                }
                if (setTileState(*tile, TileState::ready)) {
                    clearProxyTiles(tileSet, *tile, removeTiles);
                    m_tileSetChanged = true;
                }
                it = m_readyTiles.erase(it);
            } else {
                it++;
            }
        }
    }

    const std::set<TileID>& visibleTiles = m_view->getVisibleTiles();

    glm::dvec2 viewCenter(m_view->getPosition().x, -m_view->getPosition().y);

    // Tile load request above this zoom-level will be canceled in order to
    // not wait for tiles that are too small to contribute significantly to
    // the current view.
    int maxZoom = m_view->getZoom() + 2;

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
                assert(!(visTileId == NOT_A_TILE));

                // tiles in both sets match
                auto& tile = curTilesIt->second;
                tile->setVisible(true);

                if (tile->isReady() || tile->hasState(TileState::updating)) {
                    m_tiles.push_back(tile);
                } else if(tile->hasState(TileState::stale)) {
                    m_tiles.push_back(tile); // Keep on drawing the stale tile, new tile will update this
                    enqueueTask(tileSet, visTileId, viewCenter);
                    setTileState(*tile, TileState::updating);
                } else if (tile->hasState(TileState::none)) {
                    // Not yet available - enqueue for loading
                    enqueueTask(tileSet, visTileId, viewCenter);
                    if (m_tileSetChanged) {
                        // check again for proxies
                        updateProxyTiles(tileSet, *tile);
                    }
                }

                ++curTilesIt;
                ++visTilesIt;

            } else if (curTileId > visTileId) {
                // NB: if (curTileId == NOT_A_TILE) it is always > visTileId
                //     and if curTileId > visTileId, then visTileId cannot be
                //     NOT_A_TILE. (for the current implementation of > operator)
                assert(!(visTileId == NOT_A_TILE));

                // tileSet is missing an element present in visibleTiles
                if (!addTile(tileSet, visTileId)) {
                    // Not in cache - enqueue for loading
                    enqueueTask(tileSet, visTileId, viewCenter);
                }

                ++visTilesIt;

            } else {
                assert(!(curTileId == NOT_A_TILE));
                // tileSet has a tile not present in visibleTiles
                auto& tile = curTilesIt->second;

                if (!tile) {
                    curTilesIt = tiles.erase(curTilesIt);
                    continue;
                }

                if (tile->getProxyCounter() <= 0) {
                    removeTiles.push_back(tile->getID());
                } else if (tile->isReady()) {
                    m_tiles.push_back(tile);
                } else if (tile->getID().z > maxZoom) {
                    // cancel requsts for tiles that
                    // are too small on screen
                    removeTiles.push_back(tile->getID());
                }
                tile->setVisible(false);

                ++curTilesIt;
            }
        }
    }

    while (!removeTiles.empty()) {
        auto tileIt = tiles.find(removeTiles.back());
        removeTiles.pop_back();

        if ((tileIt != tiles.end()) &&
            (tileIt->second->getProxyCounter() <= 0 ||
             tileIt->second->getID().z > maxZoom)) {

            removeTile(tileSet, tileIt, removeTiles);
        }
    }

    // Update tile distance to map center for load priority
    for (auto& entry : tiles) {
        auto& tile = entry.second;
        auto tileCenter = m_view->getMapProjection().TileCenter(tile->getID());
        double scaleDiv = exp2(tile->getID().z - m_view->getZoom());
        // prefer parent tiles
        if (scaleDiv < 1) { scaleDiv = 0.1/scaleDiv; }
        tile->setPriority(glm::length2(tileCenter - viewCenter) * scaleDiv);
    }
}

void TileManager::enqueueTask(const TileSet& tileSet, const TileID& tileID,
                              const glm::dvec2& viewCenter) {

    // Keep the items sorted by distance
    auto tileCenter = m_view->getMapProjection().TileCenter(tileID);
    double distance = glm::length2(tileCenter - viewCenter);

    auto it = std::upper_bound(m_loadTasks.begin(), m_loadTasks.end(), distance,
                               [](auto& distance, auto& other){
                                   return distance < std::get<0>(other);
                               });

    // Enqueue a new tile to update the stale tile
    if (tileSet.tiles.at(tileID)->hasState(TileState::stale)) {
        std::shared_ptr<Tile> tile(new Tile(tileID, m_view->getMapProjection()));
        m_loadTasks.insert(it, std::make_tuple(distance, &tileSet, std::move(tile)));
    } else {
        m_loadTasks.insert(it, std::make_tuple(distance, &tileSet, tileSet.tiles.at(tileID)));
    }
}

void TileManager::loadTiles() {

    for (auto& item : m_loadTasks) {

        auto& tile =  std::get<2>(item);
        auto& id = tile->getID();
        auto& tileSet = *std::get<1>(item);

        auto& source = tileSet.source;

        auto task = std::make_shared<TileTask>(tile, source);
        if (source->getTileData(task)) {
            setTileState(*tile, TileState::loading);
            m_dataCallback.func(std::move(task));

        } else if (m_loadPending < (int)MAX_DOWNLOADS) {
            setTileState(*tile, TileState::loading);

            if (!source->loadTileData(std::move(task), m_dataCallback)) {
                LOGE("Loading failed for tile [%d, %d, %d]", id.z, id.x, id.y);
                m_loadPending--;
            }
        }
    }

    LOGD("loading:%d pending:%d cache: %fMB",
       m_loadTasks.size(), m_loadPending,
       (double(m_tileCache->getMemoryUsage()) / (1024 * 1024)));

    m_loadTasks.clear();
}

bool TileManager::addTile(TileSet& tileSet, const TileID& _tileID) {

    auto tile = m_tileCache->get(tileSet.source->id(), _tileID);
    bool fromCache = false;

    if (tile) {
        m_tiles.push_back(tile);

        // Update tile origin based on wrap (set in the new tileID)
        tile->updateTileOrigin(_tileID.wrap);
        // Reset tile on potential internal dynamic data set
        tile->reset();

        fromCache = true;
    } else {
        tile = std::shared_ptr<Tile>(new Tile(_tileID, m_view->getMapProjection()));
        //Add Proxy if corresponding proxy MapTile ready
        updateProxyTiles(tileSet, *tile);
    }

    tile->setVisible(true);
    tileSet.tiles.emplace(_tileID, tile);

    return fromCache;
}

void TileManager::removeTile(TileSet& tileSet, std::map<TileID,
                             std::shared_ptr<Tile>>::iterator& _tileIt,
                             std::vector<TileID>& _removes) {

    const TileID& id = _tileIt->first;
    auto& tile = _tileIt->second;

    if (tile->hasState(TileState::loading) &&
        setTileState(*tile, TileState::canceled)) {
        // 1. Remove from Datasource. Make sure to cancel the network request
        // associated with this tile.
        tileSet.source->cancelLoadingTile(id);
    }

    clearProxyTiles(tileSet, *tile, _removes);

    if (tile->hasState(TileState::ready)) {
        // Add to cache
        m_tileCache->put(tileSet.source->id(), tile);
    }

    // Remove tile from set
    _tileIt = tileSet.tiles.erase(_tileIt);
}

bool TileManager::updateProxyTile(TileSet& tileSet, Tile& _tile, const TileID& _proxy, const Tile::ProxyID _proxyID) {
    auto& tiles = tileSet.tiles;

    // check if the proxy exists in the visible tile set
    {
        const auto& it = tiles.find(_proxy);
        if (it != tiles.end()) {
            auto& proxyTile = it->second;
            if (proxyTile && _tile.setProxy(_proxyID)) {
                proxyTile->incProxyCounter();
                if (proxyTile->isReady()) {
                    m_tiles.push_back(proxyTile);
                }
                return true;
            }
        }
    }

    // check if the proxy exists in the cache
    {
        auto proxyTile = m_tileCache->get(tileSet.source->id(), _proxy);
        if (proxyTile) {
            _tile.setProxy(_proxyID);
            proxyTile->incProxyCounter();
            m_tiles.push_back(proxyTile);

            tiles.emplace(_proxy, std::move(proxyTile));
            return true;
        }
    }

    return false;
}

void TileManager::updateProxyTiles(TileSet& tileSet, Tile& _tile) {
    const TileID& _tileID = _tile.getID();

    // Get current parent
    auto parentID = _tileID.getParent();
    // Get Grand Parent
    auto gparentID = parentID.getParent();

    if (!updateProxyTile(tileSet, _tile, gparentID, Tile::parent2)) {
        if (!updateProxyTile(tileSet, _tile, parentID, Tile::parent)) {
            if (m_view->s_maxZoom > _tileID.z) {
                for (int i = 0; i < 4; i++) {
                    updateProxyTile(tileSet, _tile, _tileID.getChild(i), static_cast<Tile::ProxyID>(1 << i));
                }
            }
        }
    }
}

void TileManager::clearProxyTiles(TileSet& tileSet, Tile& _tile,
                                  std::vector<TileID>& _removes) {
    const TileID& _tileID = _tile.getID();
    auto& tiles = tileSet.tiles;

    // Check if gparent proxy is present
    if (_tile.unsetProxy(Tile::parent2)) {
        TileID gparentID(_tileID.getParent().getParent());
        auto it = tiles.find(gparentID);
        if (it != tiles.end()) {
            auto& gparent = it->second;
            gparent->decProxyCounter();

            if (gparent->getProxyCounter() == 0 && !gparent->isVisible()) {
                _removes.push_back(gparentID);
            }
        }
    }

    // Check if parent proxy is present
    if (_tile.unsetProxy(Tile::parent)) {
        TileID parentID(_tileID.getParent());
        auto it = tiles.find(parentID);
        if (it != tiles.end()) {
            auto& parent = it->second;
            parent->decProxyCounter();

            if (parent->getProxyCounter() == 0 && !parent->isVisible()) {
                _removes.push_back(parentID);
            }
        }
    }

    // Check if child proxies are present
    for (int i = 0; i < 4; i++) {
        if (_tile.unsetProxy(static_cast<Tile::ProxyID>(1 << i))) {
            TileID childID(_tileID.getChild(i));
            auto it = tiles.find(childID);

            if (it != tiles.end()) {
                auto& child = it->second;
                child->decProxyCounter();

                if (child->getProxyCounter() == 0 && !child->isVisible()) {
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
