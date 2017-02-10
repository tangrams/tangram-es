#include "data/tileSource.h"

#include "data/tileData.h"
#include "tile/tileID.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "platform.h"
#include "log.h"

#include <atomic>
#include <functional>

namespace Tangram {

TileSource::TileSource(const std::string& _name, std::unique_ptr<DataSource> _sources,
                       int32_t _minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom) :
    m_name(_name),
    m_minDisplayZoom(_minDisplayZoom),
    m_maxDisplayZoom(_maxDisplayZoom),
    m_maxZoom(_maxZoom),
    m_sources(std::move(_sources)) {

    static std::atomic<int32_t> s_serial;

    m_id = s_serial++;
}

TileSource::~TileSource() {
    clearData();
}

std::shared_ptr<TileTask> TileSource::createTask(TileID _tileId, int _subTask) {
    auto task = std::make_shared<BinaryTileTask>(_tileId, shared_from_this(), _subTask);

    createSubTasks(task);

    return task;
}

void TileSource::createSubTasks(std::shared_ptr<TileTask> _task) {
    size_t index = 0;

    for (auto& subSource : m_rasterSources) {
        TileID subTileID = _task->tileId();

        // get tile for lower zoom if we are past max zoom
        if (subTileID.z > subSource->maxZoom()) {
            subTileID = subTileID.withMaxSourceZoom(subSource->maxZoom());
        }

        _task->subTasks().push_back(subSource->createTask(subTileID, index++));
    }
}

void TileSource::clearData() {

    if (m_sources) { m_sources->clear(); }

    m_generation++;
}

bool TileSource::equals(const TileSource& other) const {
    if (m_name != other.m_name) { return false; }
    // TODO compare DataSources instead
    //if (m_urlTemplate != other.m_urlTemplate) { return false; }
    if (m_minDisplayZoom != other.m_minDisplayZoom) { return false; }
    if (m_maxDisplayZoom != other.m_maxDisplayZoom) { return false; }
    if (m_maxZoom != other.m_maxZoom) { return false; }
    if (m_rasterSources.size() != other.m_rasterSources.size()) { return false; }
    for (size_t i = 0, end = m_rasterSources.size(); i < end; ++i) {
        if (!m_rasterSources[i]->equals(*other.m_rasterSources[i])) { return false; }
    }

    return true;
}

void TileSource::loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) {

    if (m_sources) {
        if (_task->needsLoading()) {
            if (m_sources->loadTileData(_task, _cb)) {
                _task->startedLoading();
            }
        }
    }

    for (auto& subTask : _task->subTasks()) {
        subTask->source().loadTileData(subTask, _cb);
    }
}

void TileSource::cancelLoadingTile(const TileID& _tileID) {

    if (m_sources) { return m_sources->cancelLoadingTile(_tileID); }

    for (auto& raster : m_rasterSources) {
        TileID rasterID = _tileID.withMaxSourceZoom(raster->maxZoom());
        raster->cancelLoadingTile(rasterID);
    }
}

void TileSource::clearRasters() {
    for (auto& raster : m_rasterSources) {
        raster->clearRasters();
    }
}

void TileSource::clearRaster(const TileID& id) {
    for (auto& raster : m_rasterSources) {
        TileID rasterID = id.withMaxSourceZoom(raster->maxZoom());
        raster->clearRaster(rasterID);
    }
}

void TileSource::addRasterSource(std::shared_ptr<TileSource> _rasterSource) {
    /*
     * We limit the parent source by any attached raster source's min/max.
     */
    int32_t rasterMinDisplayZoom = _rasterSource->minDisplayZoom();
    int32_t rasterMaxDisplayZoom = _rasterSource->maxDisplayZoom();
    if (rasterMinDisplayZoom > m_minDisplayZoom) {
        m_minDisplayZoom = rasterMinDisplayZoom;
    }
    if (rasterMaxDisplayZoom < m_maxDisplayZoom) {
        m_maxDisplayZoom = rasterMaxDisplayZoom;
    }
    m_rasterSources.push_back(_rasterSource);
}

}
