#include "rasterSource.h"
#include "propertyItem.h"
#include "util/mapProjection.h"

#include "tileData.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/geoJson.h"
#include "platform.h"

namespace Tangram {


RasterSource::RasterSource(const std::string& _name, const std::string& _urlTemplate, int32_t _maxZoom,
                            TextureOptions _options, bool _genMipmap) :
        DataSource(_name, _urlTemplate, _maxZoom), m_texOptions(_options),m_genMipmap(_genMipmap) {
}

std::shared_ptr<TileData> RasterSource::parse(const TileTask& _task, const MapProjection& _projection) const {

    std::shared_ptr<TileData> tileData = std::make_shared<TileData>();

    Feature rasterFeature;
    rasterFeature.geometryType = GeometryType::polygons;
    rasterFeature.polygons = { { {
                                         {0.0f, 0.0f, 0.0f},
                                         {1.0f, 0.0f, 0.0f},
                                         {1.0f, 1.0f, 0.0f},
                                         {0.0f, 1.0f, 0.0f},
                                         {0.0f, 0.0f, 0.0f}
                                 } } };
    rasterFeature.props = Properties();

    tileData->layers.emplace_back("");
    tileData->layers.back().features.push_back(rasterFeature);
    return tileData;

}

std::shared_ptr<TileTask> RasterSource::createTask(TileID _tileId) {
    auto task = std::make_shared<DownloadTileTask>(_tileId, shared_from_this());

    cacheGet(*task);

    return task;
}

// Load/Download referenced raster data
bool RasterSource::onTileLoaded(std::vector<char>&& _rawData, std::shared_ptr<TileTask>&& _task,
                                TileTaskCb _cb) {

    TileID tileID = _task->tileId();

    auto rawDataRef = std::make_shared<std::vector<char>>();
    std::swap(*rawDataRef, _rawData);
    auto& task = static_cast<DownloadTileTask&>(*_task);
    task.rawTileData = rawDataRef;

    raster(*_task);
    _task->rasterReady();
    _cb.func(std::move(_task));

    // Also cache non-existent/failed tiles to not load twice
    // TODO: should differentiate between empty and failed!
    cachePut(tileID, rawDataRef);

    return true;
}

bool RasterSource::loadTileData(std::shared_ptr<TileTask>&& _task, TileTaskCb _cb) {

    std::string url(constructURL(_task->tileId()));

    auto copyTask = _task;

    // lambda captured parameters are const by default, we want "task" (moved) to be non-const,
    // hence "mutable"
    // Refer: http://en.cppreference.com/w/cpp/language/lambda
    bool status = startUrlRequest(url,
            [this, _cb, task = std::move(_task)](std::vector<char>&& rawData) mutable {
                this->onTileLoaded(std::move(rawData), std::move(task), _cb);
            });

    // For "dependent" raster datasources if this returns false make sure to create a black texture
    // for tileID in this task, and consider dependent raster ready
    if (!status) {
        copyTask->rasterReady();
    }

    return status;
}

Raster RasterSource::raster(const TileTask& _task) {

    TileID id(_task.tileId().x, _task.tileId().y, _task.tileId().z);

    unsigned char* udata = nullptr;
    size_t dataSize = 0;
    std::shared_ptr<Texture> texture;

    {
        std::lock_guard<std::mutex> lock(m_textureMutex);
        if (m_textures.find(id) != m_textures.end() && m_textures.at(id)) {
            return { id, m_textures.at(id) };
        }

        auto &task = static_cast<const DownloadTileTask &>(_task);
        if (task.rawTileData) {
            udata = (unsigned char*)task.rawTileData->data();
            dataSize = task.rawTileData->size();
        }
        texture = std::make_shared<Texture>(udata, dataSize, m_texOptions, m_genMipmap, true);

        m_textures[id] = texture;

        if (!texture->hasValidData() && dataSize > 0) {
            LOGW("Texture for data source %s has failed to decode", m_name.c_str());
        }
    }

    return { id, texture };
}

void RasterSource::clearRasters() {
    for (auto& raster: m_rasterSources) {
        raster->clearRasters();
    }

    std::lock_guard<std::mutex> lock(m_textureMutex);
    m_textures.clear();
}

void RasterSource::clearRaster(const TileID &tileID) {
    TileID id(tileID.x, tileID.y, tileID.z);

    for (auto& raster: m_rasterSources) {
        TileID rasterID = id.withMaxSourceZoom(raster->maxZoom());
        raster->clearRaster(rasterID);
    }

    auto rasterID = id.withMaxSourceZoom(m_maxZoom);

    // We do not want to delete the texture reference from the
    // DS if any of the tiles is still using this as a reference
    std::lock_guard<std::mutex> lock(m_textureMutex);
    if (m_textures[rasterID].use_count() <= 1) {
        m_textures.erase(rasterID);
    }
}

}
