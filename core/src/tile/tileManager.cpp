#include "tileManager.h"

#include "data/dataSource.h"
#include "platform.h"
#include "scene/scene.h"
#include "tile/mapTile.h"
#include "view/view.h"

#include "glm/gtx/norm.hpp"

#include <chrono>
#include <algorithm>

#define MAX_CONCURRENT_DOWNLOADS 4

TileManager::TileManager() {
    // Instantiate workers
    for (size_t i = 0; i < MAX_WORKERS; i++) {
        m_workers.push_back(std::unique_ptr<TileWorker>(new TileWorker()));
    }
}

TileManager::TileManager(TileManager&& _other) :
    m_view(std::move(_other.m_view)),
    m_tileSet(std::move(_other.m_tileSet)),
    m_dataSources(std::move(_other.m_dataSources)),
    m_workers(std::move(_other.m_workers)),
    m_queuedTiles(std::move(_other.m_queuedTiles)) {
}

TileManager::~TileManager() {
    for (auto& worker : m_workers) {
        if (!worker->isFree()) {
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

void TileManager::addToWorkerQueue(TileTask task) {
    std::lock_guard<std::mutex> lock(m_queueTileMutex);
    m_queuedTiles.push_back(task);
}

TileTask TileManager::pollTileTask() {
    std::lock_guard<std::mutex> lock(m_queueTileMutex);
    if (m_queuedTiles.empty())
         return TileTask(nullptr);

    auto task = std::move(m_queuedTiles.front());
    m_queuedTiles.pop_front();
    return task;
}

void TileManager::updateTileSet() {

    m_tileSetChanged = false;

    // Check if any native worker needs to be dispatched i.e. queuedTiles is not empty.
    if (!m_queuedTiles.empty()) {

        //logMsg("QUEUED TILES: %d\n", m_queuedTiles.size());

        for (auto& worker : m_workers) {
            if (worker->isFree()){
                auto task = pollTileTask();

                while (task) {

                    // Check if the tile was not canceled.
                    if (task->tile->state() == MapTile::Loading) {
                        // FIXME - use atomic and decrement on retrival
                        m_loadPending--;

                        task->tile->setState(MapTile::Processing);
                        worker->processTileData(std::move(task), m_scene->getStyles(), *m_view);
                        break;
                    }

                    const auto& id = task->tileID;
                    logMsg("[%d, %d, %d] >>> skipping work for removed tile\n", id.z, id.x, id.y);

                    if (task->tile->state() != MapTile::Canceled) {
                        logMsg(">>> WRONG STATE %d <<<", task->tile->state());
                        assert(false);
                    }

                    task = pollTileTask();
                }
            }
        }
    }

    // Check if any incoming tiles are finished.
    for (auto& worker : m_workers) {

        if (!worker->isFree() && worker->isFinished()) {

            // Get result from worker and move it into tile set.
            auto tile = worker->getTileResult();
            const TileID& id = tile->getID();

            logMsg("[%d, %d, %d] Tile finished loading\n", id.z, id.x, id.y);

            if (tile->state() == MapTile::Processing) {
                tile->setState(MapTile::Ready);

                cleanProxyTiles(*tile);
                m_tileSetChanged = true;
            } else if (tile->state() != MapTile::Canceled) {
                logMsg(">>> WRONG STATE %d <<<", tile->state());
                assert(false);
            }
        }
    }

    if (m_view->changedOnLastUpdate() || m_tileSetChanged) {

        const std::set<TileID>& visibleTiles = m_view->getVisibleTiles();
        bool cleanupTiles = false;

        // Loop over visibleTiles and add any needed tiles to tileSet
        {
            auto setTilesIter = m_tileSet.begin();
            auto visTilesIter = visibleTiles.begin();

            while (visTilesIter != visibleTiles.end()) {

                auto& visTileId = *visTilesIter;
                auto& curTileId = setTilesIter == m_tileSet.end() ? NOT_A_TILE : setTilesIter->first;

                if (visTileId == curTileId) {
                    if (setTilesIter->second->state() == MapTile::None){
                        //logMsg("Enqueue tile [%d, %d, %d]\n", curTileId.z, curTileId.x, curTileId.y);
                        m_loadQueue.push_back(curTileId);
                    }

                    // tiles in both sets match
                    ++setTilesIter;
                    ++visTilesIter;

                } else if (curTileId == NOT_A_TILE || visTileId < curTileId) {
                    // tileSet is missing an element present in visibleTiles
                    addTile(visTileId);
                    ++visTilesIter;

                } else {
                    // visibleTiles is missing an element present in tileSet (handled below)
                    cleanupTiles = true;
                    ++setTilesIter;
                }
            }
        }

        // Loop over tileSet and remove any tiles that are neither visible nor proxies
        if (cleanupTiles) {
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

    if (!m_loadQueue.empty() && m_loadPending < MAX_CONCURRENT_DOWNLOADS) {
        glm::dvec2 center(m_view->getPosition().x, -m_view->getPosition().y);

        m_loadQueue.sort([&](const TileID& a, const TileID& b) {
            auto ca = m_view->getMapProjection().TileCenter(a);
            auto cb = m_view->getMapProjection().TileCenter(b);
            return glm::length2(ca - center) < glm::length2(cb - center);
        });

        for (auto& id : m_loadQueue) {

            auto& tile = m_tileSet[id];

            if (tile->state() != MapTile::None) {
                logMsg(">>> WRONG STATE %d <<<", tile->state());
                assert(false);
            }

            tile->setState(MapTile::Loading);

            for (auto& source : m_dataSources) {
                //auto task = std::unique_ptr<TileTask>(new TileTask(*this, id, tile->getSerial(), source.get()));

                TileTask task = std::make_shared<TileTaskData>( *this, id, source.get() );
                task->tile = tile;

                if (!source->loadTileData(task)) {
                    logMsg("ERROR: Loading failed for tile [%d, %d, %d]\n", id.z, id.x, id.y);
                }
            }
            if (++m_loadPending == MAX_CONCURRENT_DOWNLOADS)
                break;
        }
    }
    logMsg("all:%d loading:%d pending:%d\n", m_tileSet.size(), m_loadQueue.size(), m_loadPending);
    m_loadQueue.clear();
}

void TileManager::addTile(const TileID& _tileID) {
    //logMsg("[%d, %d, %d] ADD Tile\n", _tileID.z, _tileID.x, _tileID.y);

    std::shared_ptr<MapTile> tile(new MapTile(_tileID, m_view->getMapProjection()));

    //Add Proxy if corresponding proxy MapTile ready
    updateProxyTiles(*tile);

    m_loadQueue.push_back(_tileID);
    m_tileSet[_tileID] = std::move(tile);
}

void TileManager::removeTile(std::map< TileID, std::shared_ptr<MapTile> >::iterator& _tileIter) {
    const TileID& id = _tileIter->first;
    auto& tile = _tileIter->second;

    //logMsg("[%d, %d, %d] REMOVE Tile\n", id.z, id.x, id.y);

    if (tile->state() == MapTile::Loading) {
        // 1. Remove from Datasource
        m_loadPending--;

        // Make sure to cancel the network request associated with this tile,
        // then if already fetched remove it from the processing queue and
        // the worker managing this tile, if applicable
        for(auto& dataSource : m_dataSources) {
            // FIXME ??? cleanProxyTiles(tile);
            dataSource->cancelLoadingTile(id);
        }

        // 2. Remove from tiles queued for processing
        std::lock_guard<std::mutex> lock(m_queueTileMutex);

        const auto& found = std::find_if(m_queuedTiles.begin(), m_queuedTiles.end(),
                                         [&](const TileTask& p) {
                                             return (p->tileID == id);
                                         });
        if (found != m_queuedTiles.end()) {
            m_queuedTiles.erase(found);
        }
    } else if (tile->state() == MapTile::Processing) {
        // 3. If a worker is processing this tile, abort it

        for (const auto& worker : m_workers) {
            if (!worker->isFree() && worker->getTileID() == id) {
                worker->abort();
                // Proxy tiles will be cleaned in update loop
            }
        }
    }

    tile->setState(MapTile::Canceled);

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
        if (_tile.setProxy(MapTile::Parent))
            parent->incProxyCounter();
        return;
    }

    if (m_view->s_maxZoom > _tileID.z) {
        for(int i = 0; i < 4; i++) {
            const auto& childID = _tileID.getChild(i);
            const auto& childTileIter = m_tileSet.find(childID);
            if(childTileIter != m_tileSet.end()) {
                if (_tile.setProxy((MapTile::ProxyID)(1 << i)))
                    childTileIter->second->incProxyCounter();
            }
        }
    }
}

void TileManager::cleanProxyTiles(MapTile& _tile) {
    const TileID& _tileID = _tile.getID();

    // check if parent proxy is present
    if (_tile.hasProxy(MapTile::Parent)) {
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
