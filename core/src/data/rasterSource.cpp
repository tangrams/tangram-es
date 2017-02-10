#include "data/rasterSource.h"
#include "data/propertyItem.h"
#include "data/tileData.h"
#include "tile/tile.h"
#include "tile/tileTask.h"
#include "util/geoJson.h"
#include "util/mapProjection.h"
#include "platform.h"

namespace Tangram {

class RasterTileTask : public BinaryTileTask {
public:
    RasterTileTask(TileID& _tileId, std::shared_ptr<TileSource> _source, int _subTask)
        : BinaryTileTask(_tileId, _source, _subTask) {}

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
            BinaryTileTask::process(_tileBuilder);
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


RasterSource::RasterSource(const std::string& _name, std::unique_ptr<DataSource> _sources,
                           int32_t _minDisplayZoom, int32_t _maxDisplayZoom, int32_t _maxZoom,
                           TextureOptions _options, bool _genMipmap)
    : TileSource(_name, std::move(_sources), _minDisplayZoom, _maxDisplayZoom, _maxZoom),
      m_texOptions(_options),
      m_genMipmap(_genMipmap) {

    std::vector<char> data = {};
    m_emptyTexture = std::make_shared<Texture>(data, m_texOptions, m_genMipmap);
}

std::shared_ptr<Texture> RasterSource::createTexture(const std::vector<char>& _rawTileData) {
    if (_rawTileData.size() == 0) {
        return m_emptyTexture;
    }

    auto texture = std::make_shared<Texture>(_rawTileData, m_texOptions, m_genMipmap);

    return texture;
}

void RasterSource::loadTileData(std::shared_ptr<TileTask> _task, TileTaskCb _cb) {
    // TODO, remove this
    // Overwrite cb to set empty texture on failure
    TileTaskCb cb{[this, _cb](std::shared_ptr<TileTask> _task) {

            if (!_task->hasData()) {
                auto& task = static_cast<RasterTileTask&>(*_task);
                task.m_texture = m_emptyTexture;
            }
            _cb.func(_task);
        }};

    TileSource::loadTileData(_task, cb);
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

    createSubTasks(task);

    // First try existing textures cache
    {
        TileID id(_tileId.x, _tileId.y, _tileId.z);

        auto texIt = m_textures.find(id);
        if (texIt != m_textures.end()) {
            task->m_texture = texIt->second;

            // No more loading needed.
            task->startedLoading();

            return task;
        }
    }

    return task;
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
