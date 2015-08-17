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

#define DBG(...) logMsg(__VA_ARGS__)
#define DBGC(...) logMsg(__VA_ARGS__)

const static size_t MAX_WORKERS = 2;
const static int MAX_DOWNLOADS = 4;

namespace Tangram {

TileManager::TileManager()
    : m_loadPending(0) {

    // Instantiate workers
    m_workers = std::make_unique<TileWorker>(*this, MAX_WORKERS);

    m_tileCache = std::make_unique<TileCache>();

    m_dataCallback = [this](std::shared_ptr<TileTask>&& task){
        if (setTileState(*task->tile, TileState::processing)) {
            m_workers->enqueue(std::move(task));
        }
    };
}

TileManager::~TileManager() {
    if (m_workers->isRunning()) {
        m_workers->stop();
    }
    m_tileSets.clear();
}

void TileManager::addDataSource(std::shared_ptr<DataSource>&& dataSource) {
    m_tileSets.push_back({dataSource});
}

void TileManager::tileProcessed(std::shared_ptr<TileTask>&& task) {
    std::lock_guard<std::mutex> lock(m_readyTileMutex);
    m_readyTiles.push_back(std::move(task));
}

bool TileManager::setTileState(Tile& tile, TileState state) {
    std::lock_guard<std::mutex> lock(m_tileStateMutex);

    switch (tile.getState()) {
    case TileState::none:
        if (state == TileState::loading) {
            tile.setState(state);
            m_loadPending++;
            return true;
        }
        if (state == TileState::processing) {
            tile.setState(state);
            return true;
        }
        break;

    case TileState::loading:
        if (state == TileState::processing) {
            tile.setState(state);
            m_loadPending--;
            return true;
        }
        if (state == TileState::canceled) {
            tile.setState(state);
            m_loadPending--;
            return true;
        }
        break;

    case TileState::processing:
        if (state == TileState::ready) {
            tile.setState(state);
            return true;
        }
        break;

    case TileState::canceled:
        // Ignore any state change when tile was canceled
         return false;

    default:
        break;
    }

    if (state == TileState::canceled) {
        tile.setState(state);
        return true;
    }

    logMsg("Wrong state change %d -> %d<<<", tile.getState(), state);
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

void TileManager::updateTileSets() {

    m_tileSetChanged = false;
    m_tiles.clear();

    for (auto& tileSet : m_tileSets) {
        updateTileSet(tileSet);
    }

    loadTiles();

    std::sort(m_tiles.begin(), m_tiles.end(), [](auto& a, auto& b){
            return a->getID() < b->getID();
        });

    m_tiles.erase(std::unique(m_tiles.begin(), m_tiles.end()), m_tiles.end());

    // logMsg("-----------\n");
    //  for (auto& tile : m_tiles){
    //      logMsg("Tile %d %d %d\n", tile->getID().x,tile->getID().y,tile->getID().z);
    //  }
}

void TileManager::updateTileSet(TileSet& tileSet) {

    auto& tiles = tileSet.tiles;

    std::vector<TileID> removeTiles;

    if (!m_readyTiles.empty()) {
        std::lock_guard<std::mutex> lock(m_readyTileMutex);
        auto it = m_readyTiles.begin();

        while (it != m_readyTiles.end()) {
            auto& task = *it;
            auto& tile = *(task->tile);

            if (tileSet.source.get() == task->source) {

                if (setTileState(tile, TileState::ready)) {
                    clearProxyTiles(tileSet, tile, removeTiles);
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

    if (m_view->changedOnLastUpdate() || m_tileSetChanged) {

        // Loop over visibleTiles and add any needed tiles to tileSet
        auto setTilesIter = tiles.begin();
        auto visTilesIter = visibleTiles.begin();

        while (visTilesIter != visibleTiles.end()) {

            auto& visTileId = *visTilesIter;
            auto& curTileId = setTilesIter == tiles.end() ? NOT_A_TILE : setTilesIter->first;
            //DBG("visible: [%d, %d, %d]\n", visTileId.z, visTileId.x, visTileId.y);

            if (visTileId == curTileId) {
                auto& tile = setTilesIter->second;

                if (tile->hasState(TileState::none)) {
                    enqueueLoadTask(tileSet, visTileId, viewCenter);
                } else if (tile->isReady()) {
                    m_tiles.push_back(tile);
                }

                tile->setVisible(true);

                // tiles in both sets match
                ++setTilesIter;
                ++visTilesIter;

            } else if (curTileId == NOT_A_TILE || visTileId < curTileId) {
                // tileSet is missing an element present in visibleTiles
                if (!addTile(tileSet, visTileId)) {
                    enqueueLoadTask(tileSet, visTileId, viewCenter);
                }

                ++visTilesIter;
            } else {
                // visibleTiles is missing an element present in tileSet
                auto& tile = setTilesIter->second;
                tile->setVisible(false);
                if (tile->getProxyCounter() <= 0) {
                    removeTiles.push_back(tile->getID());
                } else if (tile->isReady()) {
                    m_tiles.push_back(tile);
                }

                ++setTilesIter;
            }
        }
        while (setTilesIter != tiles.end()) {
            // more visibleTiles missing an element present in tileSet
            auto& tile = setTilesIter->second;
            tile->setVisible(false);
            if (tile->getProxyCounter() <= 0) {
                removeTiles.push_back(tile->getID());
            } else if (tile->isReady()) {
                m_tiles.push_back(tile);
            }

            ++setTilesIter;
        }
    }

    {
        while (!removeTiles.empty()) {
            auto tileIter = tiles.find(removeTiles.back());
            removeTiles.pop_back();

            if (tileIter != tiles.end()) {
                if (tileIter->second->getProxyCounter() <= 0) {
                    removeTile(tileSet, tileIter, removeTiles);
                }
            }
        }
    }

    // Update tile distance to map center for load priority
    {
        for (auto& entry : tiles) {
            auto& tile = entry.second;
            auto tileCenter = m_view->getMapProjection().TileCenter(tile->getID());
            tile->setPriority(glm::length2(tileCenter - viewCenter));
        }
    }
}

void TileManager::enqueueLoadTask(const TileSet& tileSet, const TileID& tileID,
                                  const glm::dvec2& viewCenter) {
    // Keep the items sorted by distance
    auto tileCenter = m_view->getMapProjection().TileCenter(tileID);
    double distance = glm::length2(tileCenter - viewCenter);

    auto iter = m_loadTasks.begin();
    while (iter != m_loadTasks.end()) {
        if (std::get<0>(*iter) > distance) {
            break;
        }
        ++iter;
    }
    if (iter == m_loadTasks.end()) {
        m_loadTasks.push_back(std::make_tuple(distance, &tileSet, &tileID));
    } else {
        m_loadTasks.insert(iter, std::make_tuple(distance, &tileSet, &tileID));
    }
}

void TileManager::loadTiles() {

    for (auto& item : m_loadTasks) {
        auto& id = *std::get<2>(item);
        auto& tileSet = *std::get<1>(item);
        auto it = tileSet.tiles.find(id);
        if (it == tileSet.tiles.end()) {
            DBG("[%d, %d, %d] eeek\n", id.z, id.x, id.y);
            continue;
        }

        auto& tile = it->second;
        auto& source = tileSet.source;

        auto task = std::make_shared<TileTask>(tile, source.get());
        //DBG("[%d, %d, %d] Load\n", id.z, id.x, id.y);

        if (source->getTileData(task)) {
            DBGC("USE RAW CACHE\n");
            m_dataCallback(std::move(task));

        } else if (m_loadPending < MAX_DOWNLOADS) {
            setTileState(*tile, TileState::loading);

            if (!source->loadTileData(std::move(task), m_dataCallback)) {
                logMsg("ERROR: Loading failed for tile [%d, %d, %d]\n",
                       id.z, id.x, id.y);
                m_loadPending--;
            }
        }
    }

    // DBG("loading:%d pending:%d cached:%d cache: %fMB\n",
    //     m_loadTasks.size(),
    //     m_loadPending, m_tileCache.size(),
    //     (double(m_tileCache.getMemoryUsage()) / (1024 * 1024)));

    m_loadTasks.clear();
}

bool TileManager::addTile(TileSet& tileSet, const TileID& _tileID) {
    DBG("[%d, %d, %d] Add\n", _tileID.z, _tileID.x, _tileID.y);
    auto tile = m_tileCache->get(*tileSet.source.get(), _tileID);
    bool fromCache = false;

    if (tile) {
        m_tiles.push_back(tile);
        DBGC("USING CACHED TILE\n");
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
                             std::shared_ptr<Tile>>::iterator& _tileIter,
                             std::vector<TileID>& _removes) {

    const TileID& id = _tileIter->first;
    auto& tile = _tileIter->second;

    DBG("[%d, %d, %d] Remove\n", id.z, id.x, id.y);

    if (tile->hasState(TileState::loading) &&
        setTileState(*tile, TileState::canceled)) {
        // 1. Remove from Datasource. Make sure to cancel the network request
        // associated with this tile.
        tileSet.source->cancelLoadingTile(id);
    }

    clearProxyTiles(tileSet, *tile, _removes);

    if (tile->hasState(TileState::ready)) {
        // Add to cache
        m_tileCache->put(*tileSet.source.get(), tile);
    }

    // Remove tile from set
    _tileIter = tileSet.tiles.erase(_tileIter);
}

void TileManager::updateProxyTiles(TileSet& tileSet, Tile& _tile) {
    const TileID& _tileID = _tile.getID();
    auto& tiles = tileSet.tiles;

    // Get current parent
    auto parentID = _tileID.getParent();
    {
        const auto& it = tiles.find(parentID);
        if (it != tiles.end()) {
            auto& parent = it->second;
            if (_tile.setProxy(Tile::parent)) {
                parent->incProxyCounter();
                DBGC("USE PARENT PROXY\n");

                if (parent->isReady()) {
                    m_tiles.push_back(parent);
                }
            }
            return;
        }
    }
    // Get parent proxy from cache
    {
        auto parent = m_tileCache->get(*tileSet.source.get(), parentID);
        if (parent) {
            DBGC("USE CACHED PARENT PROXY\n");

            _tile.setProxy(Tile::parent);
            parent->incProxyCounter();
            m_tiles.push_back(parent);

            tiles.emplace(parentID, std::move(parent));

            return;
        }
    }

    if (m_view->s_maxZoom > _tileID.z) {
        for (int i = 0; i < 4; i++) {
            auto childID = _tileID.getChild(i);

            const auto& it = tiles.find(childID);
            if (it != tiles.end()) {
                auto& child = it->second;

                if (_tile.setProxy(static_cast<Tile::ProxyID>(1 << i))) {
                    child->incProxyCounter();

                    if (child->isReady()) {
                        m_tiles.push_back(child);
                    }

                    DBGC("USE CHILD PROXY\n");
                }
            } else {
                auto child = m_tileCache->get(*tileSet.source.get(), childID);
                if (child) {
                    DBGC("USE CACHED CHILD PROXY\n");
                    _tile.setProxy(static_cast<Tile::ProxyID>(1 << i));
                    child->incProxyCounter();
                    m_tiles.push_back(child);

                    tiles.emplace(childID, std::move(child));
                }
            }
        }
    }
}

void TileManager::clearProxyTiles(TileSet& tileSet, Tile& _tile, std::vector<TileID>& _removes) {
    const TileID& _tileID = _tile.getID();
    auto& tiles = tileSet.tiles;

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

}
