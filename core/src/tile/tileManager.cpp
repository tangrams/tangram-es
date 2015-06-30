#include "tileManager.h"

#include "data/dataSource.h"
#include "platform.h"
#include "scene/scene.h"
#include "tile/mapTile.h"
#include "view/view.h"

#include "glm/gtx/norm.hpp"

#include <chrono>
#include <algorithm>

TileManager::TileManager()
    : m_loadPending(0) {

    // Instantiate workers
    for (size_t i = 0; i < MAX_WORKERS; i++) {
        m_workers.push_back(std::unique_ptr<TileWorker>(new TileWorker(*this)));
    }
    m_dataCallback = std::bind(&TileManager::tileLoaded, this, std::placeholders::_1);
}

TileManager::~TileManager() {
    for (auto& worker : m_workers) {
        if (worker->isRunning()) {
            worker->abort();
            worker->drain();
        }
        // We stop all workers before we destroy the resources they use.
        // TODO: This will wait for any pending network requests to finish,
        // which could delay closing of the application.
    }
    m_dataSources.clear();
    m_tileSet.clear();
}

void TileManager::addDataSource(std::unique_ptr<DataSource> _source) { 
    m_dataSources.push_back(std::move(_source));
}

void TileManager::tileLoaded(TileTask task) {
    std::lock_guard<std::mutex> lock(m_queueTileMutex);
    m_queuedTiles.push_back(std::move(task));
}

void TileManager::tileProcessed(TileTask task) {
    std::lock_guard<std::mutex> lock(m_readyTileMutex);
    m_readyTiles.push_back(std::move(task));
}

TileTask TileManager::pollProcessQueue() {
    std::lock_guard<std::mutex> lock(m_queueTileMutex);

    while (!m_queuedTiles.empty()) {
        auto task = std::move(m_queuedTiles.front());
        m_queuedTiles.pop_front();

        if (!setTileState(*(task->tile), TileState::processing)) {
            // Drop canceled task.
            continue;
        }
        return task;
    }

    return TileTask(nullptr);
}

bool TileManager::setTileState(MapTile& tile, TileState state) {
    std::lock_guard<std::mutex> lock(m_tileStateMutex);

    switch (tile.state()) {
    case TileState::none:
        if (state == TileState::loading) {
            tile.setState(state);
            m_loadPending++;
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
         return false;

    default:
        break;
    }

    if (state == TileState::canceled) {
        return false;
    }

    logMsg("Wrong state change %d -> %d<<<", tile.state(), state);
    assert(false);
    return false; // ...
}

void TileManager::enqueueLoadTask(const TileID& tileID, const glm::dvec2& viewCenter) {
    // Keep the items sorted by distance and limit list to MAX_DOWNLOADS

    auto tileCenter = m_view->getMapProjection().TileCenter(tileID);
    double distance = glm::length2(tileCenter - viewCenter);

    bool isFull = m_loadTasks.size() == MAX_DOWNLOADS;
    if (isFull && m_loadTasks.back().first < distance) {
        return;
    }

    auto iter = m_loadTasks.begin();
    while (iter != m_loadTasks.end()){
        if (iter->first > distance) {
            break;
        }
        ++iter;
    }

    if (!isFull || iter != m_loadTasks.end()) {
        m_loadTasks.insert(iter, { distance, &tileID });
    }
    if (isFull) {
        m_loadTasks.pop_back();
    }
}

void TileManager::updateTileSet() {

    m_tileSetChanged = false;

    // Check if any native worker needs to be dispatched.
    if (!m_queuedTiles.empty()) {

        for (auto& worker : m_workers) {
            if (worker->isRunning()) { continue; }

            worker->process(m_scene->getStyles());
        }
    }

    if (!m_readyTiles.empty()) {
        std::lock_guard<std::mutex> lock(m_readyTileMutex);
        auto it = m_readyTiles.begin();

        while (it != m_readyTiles.end()) {
            auto& task = *it;
            auto& tile = *(task->tile);

            if (setTileState(tile, TileState::ready)) {
                cleanProxyTiles(tile);
                m_tileSetChanged = true;
            }

            it = m_readyTiles.erase(it);
        }
    }

    if (m_view->changedOnLastUpdate() || m_tileSetChanged) {

        glm::dvec2 viewCenter(m_view->getPosition().x, -m_view->getPosition().y);

        const std::set<TileID>& visibleTiles = m_view->getVisibleTiles();

        // Loop over visibleTiles and add any needed tiles to tileSet
        {
            auto setTilesIter = m_tileSet.begin();
            auto visTilesIter = visibleTiles.begin();

            while (visTilesIter != visibleTiles.end()) {

                auto& visTileId = *visTilesIter;
                auto& curTileId = setTilesIter == m_tileSet.end() ? NOT_A_TILE : setTilesIter->first;

                if (visTileId == curTileId) {
                    if (setTilesIter->second->state() == TileState::none) {
                        //logMsg("[%d, %d, %d] - Enqueue\n", curTileId.z, curTileId.x, curTileId.y);
                        enqueueLoadTask(visTileId, viewCenter);
                    }

                    // tiles in both sets match
                    ++setTilesIter;
                    ++visTilesIter;

                } else if (curTileId == NOT_A_TILE || visTileId < curTileId) {
                    // tileSet is missing an element present in visibleTiles
                    addTile(visTileId);
                    enqueueLoadTask(visTileId, viewCenter);

                    ++visTilesIter;
                } else {
                    // visibleTiles is missing an element present in tileSet (handled below)
                    ++setTilesIter;
                }
            }
        }

        // Loop over tileSet and remove any tiles that are neither visible nor proxies
        {
            auto setTilesIter = m_tileSet.begin();
            auto visTilesIter = visibleTiles.begin();

            while (setTilesIter != m_tileSet.end()) {

                auto& visTileId = visTilesIter == visibleTiles.end() ? NOT_A_TILE : *visTilesIter;
                auto& curTileId = setTilesIter->first;

                if (visTileId == curTileId) {
                    // tiles in both sets match
                    ++setTilesIter;
                    ++visTilesIter;

                } else if (visTileId == NOT_A_TILE || curTileId < visTileId) {
                    // remove element from tileSet not present in visibleTiles
                    const auto& tile = setTilesIter->second;
                    if (tile->getProxyCounter() <= 0) {
                        removeTile(setTilesIter);
                    } else {
                        ++setTilesIter;
                    }
                } else {
                    // tileSet is missing an element present in visibleTiles (shouldn't occur)
                    ++visTilesIter;
                }
            }
        }
    }

    if (m_loadPending < MAX_DOWNLOADS) {

        for (auto& item : m_loadTasks) {
            auto& id = *item.second;
            auto& tile = m_tileSet[id];

            setTileState(*tile, TileState::loading);

            for (auto& source : m_dataSources) {
                TileTask task = std::make_shared<TileTaskData>(tile, source.get());
                logMsg("[%d, %d, %d] Load\n", id.z, id.x, id.y);

                if (!source->loadTileData(task, m_dataCallback)) {
                    logMsg("ERROR: Loading failed for tile [%d, %d, %d]\n", id.z, id.x, id.y);
                }
            }
            if (m_loadPending == MAX_DOWNLOADS) {
                break;
            }
        }
    }
    m_loadTasks.clear();

    // logMsg("all:%d loading:%d processing:%d pending:%d\n",
    //        m_tileSet.size(), m_loadTasks.size(),
    //        m_queuedTiles.size(), m_loadPending);
}

void TileManager::addTile(const TileID& _tileID) {
    logMsg("[%d, %d, %d] Add\n", _tileID.z, _tileID.x, _tileID.y);

    std::shared_ptr<MapTile> tile(new MapTile(_tileID, m_view->getMapProjection()));

    //Add Proxy if corresponding proxy MapTile ready
    updateProxyTiles(*tile);

    m_tileSet[_tileID] = std::move(tile);
}

void TileManager::removeTile(std::map< TileID, std::shared_ptr<MapTile> >::iterator& _tileIter) {
    const TileID& id = _tileIter->first;
    auto& tile = _tileIter->second;

    logMsg("[%d, %d, %d] Remove\n", id.z, id.x, id.y);

    if (setTileState(*tile, TileState::canceled)) {

        // 1. Remove from Datasource
        // Make sure to cancel the network request associated with this tile.
        for(auto& dataSource : m_dataSources) {
            dataSource->cancelLoadingTile(id);
        }
    }

    cleanProxyTiles(*tile);

    // Remove tile from set
    _tileIter = m_tileSet.erase(_tileIter);

}

void TileManager::updateProxyTiles(MapTile& _tile) {
    const TileID& _tileID = _tile.getID();

    const auto& parentID = _tileID.getParent();
    const auto& parentTileIter = m_tileSet.find(parentID);
    if (parentTileIter != m_tileSet.end()) {
        auto& parent = parentTileIter->second;
        if (_tile.setProxy(MapTile::parent)) {
            parent->incProxyCounter();
        }
        return;
    }

    if (m_view->s_maxZoom > _tileID.z) {
        for(int i = 0; i < 4; i++) {
            const auto& childID = _tileID.getChild(i);
            const auto& childTileIter = m_tileSet.find(childID);
            if(childTileIter != m_tileSet.end()) {
                if (_tile.setProxy((MapTile::ProxyID)(1 << i))) {
                    childTileIter->second->incProxyCounter();
                }
            }
        }
    }
}

void TileManager::cleanProxyTiles(MapTile& _tile) {
    const TileID& _tileID = _tile.getID();

    // check if parent proxy is present
    if (_tile.hasProxy(MapTile::parent)) {
        const auto& parentID = _tileID.getParent();
        const auto& parentTileIter = m_tileSet.find(parentID);
        if (parentTileIter != m_tileSet.end()) {
            parentTileIter->second->decProxyCounter();
        }
    }

    // check if child proxies are present
    for(int i = 0; i < 4; i++) {
        if (_tile.hasProxy((MapTile::ProxyID)(1 << i))) {
            const auto& childID = _tileID.getChild(i);
            const auto& childTileIter = m_tileSet.find(childID);
            if (childTileIter != m_tileSet.end()) {
                childTileIter->second->decProxyCounter();
            }
        }
    }
    _tile.clearProxies();
}
