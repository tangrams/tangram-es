#include "rasterSource.h"
#include "propertyItem.h"
#include "util/mapProjection.h"

#include "tileData.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/geoJson.h"
#include "platform.h"

namespace Tangram {

class RasterTileTask : public DownloadTileTask {
public:
    RasterTileTask(TileID& _tileId, std::shared_ptr<DataSource> _source, int _subTask)
        : DownloadTileTask(_tileId, _source, _subTask) {}

    std::shared_ptr<Texture> m_texture;

    bool hasData() const override {
        return bool(rawTileData) || bool(m_texture);
    }

    bool isReady() const override {
        if (!isSubTask()) {
            return bool(m_tile);
        } else {
            return bool(m_texture);
        }
    }

    void process(TileBuilder& _tileBuilder) override {

        auto source = reinterpret_cast<RasterSource*>(m_source.get());

        if (!m_texture) {
            // Decode texture data
            m_texture = source->createTexture(*rawTileData);
        }

        // Create tile geometries
        if (!isSubTask()) {
            DownloadTileTask::process(_tileBuilder);
        }
    }

    void complete() override {
        auto source = reinterpret_cast<RasterSource*>(m_source.get());

        auto raster = source->getRaster(*this);
        assert(raster.isValid());

        m_tile->rasters().push_back(std::move(raster));

        for (auto& subTask : m_subTasks) {
            assert(subTask->isReady());
            subTask->complete(*this);
        }
    }

    void complete(TileTask& _mainTask) override {
        auto source = reinterpret_cast<RasterSource*>(m_source.get());

        auto raster = source->getRaster(*this);
        assert(raster.isValid());

        _mainTask.tile()->rasters().push_back(std::move(raster));
    }
};


RasterSource::RasterSource(const std::string& _name, const std::string& _urlTemplate, int32_t _minDisplayZoom, int32_t _maxZoom,
                           TextureOptions _options, bool _genMipmap)
    : DataSource(_name, _urlTemplate, _minDisplayZoom, _maxZoom), m_texOptions(_options), m_genMipmap(_genMipmap) {

    m_emptyTexture = std::make_shared<Texture>(nullptr, 0, m_texOptions, m_genMipmap);
}

std::shared_ptr<Texture> RasterSource::createTexture(const std::vector<char>& _rawTileData) {
    auto udata = reinterpret_cast<const unsigned char*>(_rawTileData.data());
    size_t dataSize = _rawTileData.size();

    if (dataSize == 0) {
        return m_emptyTexture;
    }

    auto texture = std::make_shared<Texture>(udata, dataSize, m_texOptions, m_genMipmap);

    return texture;
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

std::shared_ptr<TileTask> RasterSource::createTask(TileID _tileId, int _subTask) {
    auto task = std::make_shared<RasterTileTask>(_tileId, shared_from_this(), _subTask);

    // First try existing textures cache
    {
        TileID id(_tileId.x, _tileId.y, _tileId.z);

        auto texIt = m_textures.find(id);
        if (texIt != m_textures.end()) {
            task->m_texture = texIt->second;
            return task;
        }
    }

    // Try raw data cache
    cacheGet(*task);

    return task;
}

void RasterSource::onTileLoaded(std::vector<char>&& _rawData, std::shared_ptr<TileTask>&& _task,
                                TileTaskCb _cb) {

    if (_task->isCanceled()) { return; }

    TileID tileID = _task->tileId();

    auto rawDataRef = std::make_shared<std::vector<char>>();
    std::swap(*rawDataRef, _rawData);

    auto& task = static_cast<DownloadTileTask&>(*_task);
    task.rawTileData = rawDataRef;

    _cb.func(std::move(_task));

    cachePut(tileID, rawDataRef);
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
        auto& task = static_cast<RasterTileTask&>(*copyTask);
        task.m_texture = m_emptyTexture;
    }

    return status;
}

void RasterSource::loadEmptyTexture(std::shared_ptr<TileTask>&& _task) {
    auto& task = static_cast<RasterTileTask&>(*_task);
    task.m_texture = m_emptyTexture;
}

Raster RasterSource::getRaster(const TileTask& _task) {
    TileID id(_task.tileId().x, _task.tileId().y, _task.tileId().z);

    auto texIt = m_textures.find(id);
    if (texIt != m_textures.end()) {
        return { id, texIt->second };
    }

    auto& task = static_cast<const RasterTileTask&>(_task);
    m_textures.emplace(id, task.m_texture);

    return { id, task.m_texture };
}

void RasterSource::clearRasters() {
    for (auto& raster: m_rasterSources) {
        raster->clearRasters();
    }

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
    if (m_textures[rasterID].use_count() <= 1) {
        m_textures.erase(rasterID);
    }
}

}
