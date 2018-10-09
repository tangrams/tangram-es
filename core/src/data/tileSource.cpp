#include "data/tileSource.h"

#include "data/formats/geoJson.h"
#include "data/formats/mvt.h"
#include "data/formats/topoJson.h"
#include "data/tileData.h"
#include "platform.h"
#include "tile/tileID.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "log.h"
#include "util/geom.h"

#include <atomic>
#include <functional>

namespace Tangram {

TileSource::TileSource(const std::string& _name, std::unique_ptr<DataSource> _sources,
                       ZoomOptions _zoomOptions) :
    m_name(_name),
    m_zoomOptions(_zoomOptions),
    m_sources(std::move(_sources)) {

    static std::atomic<int32_t> s_serial;

    m_id = s_serial++;
}

TileSource::~TileSource() {
    clearData();
}

int32_t TileSource::zoomBiasFromTileSize(int32_t tileSize) {
    const auto BaseTileSize = 256;

    // zero tileSize (log(0) is undefined)
    if (!tileSize) { return 0; }

    if (isPowerOfTwo(tileSize)) {
        return std::log(static_cast<float>(tileSize)/static_cast<float>(BaseTileSize)) / std::log(2);
    }

    LOGW("Illegal tile_size defined. Must be power of 2. Default tileSize of 256px set");
    return 0;
}

const char* TileSource::mimeType() const {
    switch (m_format) {
    case Format::GeoJson: return "application/geo+json";
    case Format::TopoJson: return "application/topo+json";
    case Format::Mvt: return "application/vnd.mapbox-vector-tile";
    }
    assert(false);
    return "";
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

void TileSource::loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) {

    if (m_sources) {
        if (_task->needsLoading()) {
            if (m_sources->loadTileData(_task, _cb)) {
                _task->startedLoading();
            }
        } else if(_task->hasData()) {
            _cb.func(_task);
        }
    }

    for (auto& subTask : _task->subTasks()) {
        subTask->source().loadTileData(subTask, _cb);
    }
}

std::shared_ptr<TileData> TileSource::parse(const TileTask& _task) const {
    switch (m_format) {
    case Format::TopoJson: return TopoJson::parseTile(_task, m_id);
    case Format::GeoJson: return GeoJson::parseTile(_task, m_id);
    case Format::Mvt: return Mvt::parseTile(_task, m_id);
    }
    assert(false);
    return nullptr;
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
    if (rasterMinDisplayZoom > m_zoomOptions.minDisplayZoom) {
        m_zoomOptions.minDisplayZoom = rasterMinDisplayZoom;
    }
    if (rasterMaxDisplayZoom < m_zoomOptions.maxDisplayZoom) {
        m_zoomOptions.maxDisplayZoom = rasterMaxDisplayZoom;
    }
    m_rasterSources.push_back(_rasterSource);
}

}
