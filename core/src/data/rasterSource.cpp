#include "data/rasterSource.h"
#include "data/propertyItem.h"
#include "data/tileData.h"
#include "tile/tile.h"
#include "tile/tileBuilder.h"
#include "tile/tileTask.h"
#include "util/mapProjection.h"
#include "log.h"

namespace Tangram {

class RasterTileTask : public BinaryTileTask {
public:

    const bool subTask = false;

    RasterTileTask(TileID& _tileId, std::shared_ptr<TileSource> _source, bool _subTask)
        : BinaryTileTask(_tileId, _source),
          subTask(_subTask) {}

    std::shared_ptr<Texture> m_texture;

    std::shared_ptr<RasterSource> rasterSource() {
        return reinterpret_cast<std::weak_ptr<RasterSource>*>(&m_source)->lock();
    }

    bool hasData() const override {
        return bool(rawTileData) || bool(m_texture);
    }

    bool isReady() const override {
        if (!subTask) {
            return bool(m_tile);
        } else {
            return bool(m_texture);
        }
    }

    void process(TileBuilder& _tileBuilder) override {
        auto source = rasterSource();
        if (!source) { return; }

        if (!m_texture) {
            // Decode texture data
            m_texture = source->createTexture(m_tileId, *rawTileData);
        }

        // Create tile geometries
        if (!subTask) {
            m_tile = _tileBuilder.build(m_tileId, *(source->m_tileData), *source);
            m_ready = true;
        }
    }

    void complete() override {
        auto source = rasterSource();
        if (!source) { return; }

        auto raster = source->addRaster(m_tileId, m_texture);
        assert(raster.isValid());

        m_tile->rasters().push_back(std::move(raster));

        for (auto& subTask : m_subTasks) {
            assert(subTask->isReady());
            subTask->complete(*this);
        }
    }

    void complete(TileTask& _mainTask) override {
        auto source = rasterSource();
        if (!source) { return; }

        auto raster = source->addRaster(m_tileId, m_texture);
        assert(raster.isValid());

        _mainTask.tile()->rasters().push_back(std::move(raster));
    }
};


RasterSource::RasterSource(const std::string& _name, std::unique_ptr<DataSource> _sources,
                           TextureOptions _options, TileSource::ZoomOptions _zoomOptions)
    : TileSource(_name, std::move(_sources), _zoomOptions),
      m_texOptions(_options) {

    m_emptyTexture = std::make_shared<Texture>(m_texOptions);

    GLubyte pixel[4] = { 0, 0, 0, 0 };
    auto bpp = _options.bytesPerPixel();
    m_emptyTexture->setPixelData(1, 1, bpp, pixel, bpp);

    Feature rasterFeature;
    rasterFeature.geometryType = GeometryType::polygons;
    rasterFeature.polygons = { { {
                {0.0f, 0.0f},
                {1.0f, 0.0f},
                {1.0f, 1.0f},
                {0.0f, 1.0f},
                {0.0f, 0.0f}
            } } };
    rasterFeature.props = Properties();

    m_tileData = std::make_shared<TileData>();
    m_tileData->layers.emplace_back("");
    m_tileData->layers.back().features.push_back(rasterFeature);
}

std::shared_ptr<Texture> RasterSource::createTexture(TileID _tile, const std::vector<char>& _rawTileData) {
    if (_rawTileData.empty()) {
        return m_emptyTexture;
    }

    auto data = reinterpret_cast<const uint8_t*>(_rawTileData.data());
    auto length = _rawTileData.size();

    return std::make_shared<Texture>(data, length, m_texOptions);
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

std::shared_ptr<TileData> RasterSource::parse(const TileTask& _task) const {
    assert(false);
    return nullptr;
}

void RasterSource::addRasterTask(TileTask& _task) {

    TileID subTileID = _task.tileId();

    // get tile for lower zoom if we are past max zoom
    if (subTileID.z > maxZoom()) {
        subTileID = subTileID.withMaxSourceZoom(maxZoom());
    }

    auto rasterTask = createRasterTask(subTileID, true);

    _task.subTasks().push_back(rasterTask);

    // Needed? Do we support recursive raster-source inclusion?
    addRasterTasks(_task);
}

std::shared_ptr<RasterTileTask> RasterSource::createRasterTask(TileID _tileId, bool subTask) {
    auto task = std::make_shared<RasterTileTask>(_tileId, shared_from_this(), subTask);

    // First try existing textures cache
    TileID id(_tileId.x, _tileId.y, _tileId.z);

    auto texIt = m_textures.find(id);
    if (texIt != m_textures.end()) {
        task->m_texture = texIt->second;

        if (task->m_texture) {
            // No more loading needed.
            task->startedLoading();
        }
    }
    return task;
}

std::shared_ptr<TileTask> RasterSource::createTask(TileID _tileId) {
    auto task = createRasterTask(_tileId, false);

    addRasterTasks(*task);

    return task;
}

Raster RasterSource::addRaster(const TileID& _tileId, std::shared_ptr<Texture> _texture) {
    TileID id(_tileId.x, _tileId.y, _tileId.z);

    if (_texture == m_emptyTexture) {
        return Raster{id, m_emptyTexture};
    }

    auto entry = m_textures.emplace(id, _texture);
    std::shared_ptr<Texture> texture = entry.first->second;
    LOGD("Cache: add %d/%d/%d - reused: %d", id.x, id.y, id.z, !entry.second);

    // Remove textures that are only held by cache
    for(auto it = m_textures.begin(), end = m_textures.end(); it != end; ) {
        if (it->second.use_count() == 1) {
            LOGD("Cache: remove %d/%d/%d", it->first.x, it->first.y, it->first.z);
            it = m_textures.erase(it);
        } else {
            ++it;
        }
    }

    return Raster{id, texture};
}

}
